# Phase 209 - Seam B Mitigation (MIT-02): Pack camera UBO into one updateDynamicBuffer

## Context

Phase 207 traced the historical D3D12 0xC0000005 to three candidate seams.
This phase mitigates **Seam B**: three separate `updateDynamicBuffer` calls
issued on the *same* uniform buffer in `uploadCameraUniform`. Each call is a
sub-range write (offset 0/64/76). In QRhi's D3D12 backend each
`updateDynamicBuffer` enqueues a distinct upload-record; three records on one
buffer for a single frame is a known D3D12 hazard area (the backend must
coalesce them into a single upload buffer + barrier, and an off-by-one in the
coalescing is a plausible source of an access violation during SRB bind).

Packing all three sub-ranges into one CPU-side POD struct and issuing a single
`updateDynamicBuffer(buf, 0, sizeof(Packed), &packed)` is strictly more
correct under std140 layout, strictly more efficient (one upload record), and
eliminates the multi-record coalescing path entirely.

## Problem location

`src/qml_gui/Renderer/RhiViewportRenderer.cpp`, `uploadCameraUniform`
(~:1620-1639):

```cpp
updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 0, 64,
                             corrected.constData());
...
const float gizmoScale = std::max((m_gizmoCenter - m_cameraEye).length()
                                  * 0.15f, 5.f);
updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 64, 12,
                             &m_gizmoCenter[0]);
updates->updateDynamicBuffer(m_cameraUniformBuffer.get(), 76, 4,
                             &gizmoScale);
```

## Mitigation

Define a POD struct matching the GLSL `std140` `CameraBlock` and issue one
`updateDynamicBuffer`.

### Design decisions: struct layout (no padding adjustment needed)

The existing code comment (~:1623-1632) documents the layout exactly. Under
GLSL `std140`:

- `mat4 mvp`        -> offset 0,  64 bytes (4 columns x vec4)
- `vec3 gizmoCenter`-> offset 64, 12 bytes of data, but std140 rounds the
                       *member stride* to 16 bytes; the next member (float)
                       occupies the tail 4 bytes of that 16-byte slot
- `float gizmoScale`-> offset 76, 4 bytes

So `gizmoCenter.x/y/z` are at bytes 64/68/72, `gizmoScale` at bytes 76-79,
total 80 bytes. **The C++ struct layout is bit-identical to std140 here
without any manual padding**, because:

- `QMatrix4x4` is 16 contiguous floats (64 bytes), alignment 4 (float) - OK.
- `QVector3D` is 3 contiguous floats (12 bytes), alignment 4 - placed at
  offset 64, no padding gap (64 is already 16-aligned? no, 64 % 4 == 0 is
  enough for float alignment; std140 wants 16 for the vec3 *base*, and 64
  is 16-aligned, so the C++ struct and std140 agree).
- `float gizmoScale` follows at offset 76 - no gap, 4-byte aligned.

A `static_assert(sizeof(CameraBlockPacked) == 80)` documents the invariant
and will fail to compile if any future change breaks it (e.g. a Qt ABI change
to QMatrix4x4 / QVector3D, which won't happen). No padding member is added -
the existing code already relies on the exact same byte layout, we are only
coalescing the *issue* of it.

### Changes

**`src/qml_gui/Renderer/RhiViewportRenderer.cpp`**
- At file scope (after the `rhiTrace` anon-namespace, before the ctor, ~:33),
  add the POD struct with a `static_assert`.
- Replace the three `updateDynamicBuffer` calls with one. Keep the
  `gizmoScale` computation (`std::max(...)`) verbatim; only the destination
  changes (struct field vs local). Add `rhiTrace("seamB-packed")`.

**`src/qml_gui/Renderer/RhiViewportRenderer.h`**
- No changes (struct is local to the .cpp - it is an implementation detail
  of the upload path, not part of the renderer's API).

## Verification (static only)

- `static_assert(sizeof(CameraBlockPacked) == 80)` enforces the invariant at
  compile time.
- `corrected` is `QMatrix4x4`; `corrected.constData()` returns
  `const float*` to 16 floats - assignable to a `QMatrix4x4` by value (Qt's
  implicit-sharing copy is cheap; the bytes then live in `packed`).
- `&m_gizmoCenter[0]` -> `m_gizmoCenter` direct assignment (QVector3D copy).
- D3D11 path: identical bytes uploaded, one record instead of three; no
  behavioral change, marginally faster.
- The mesh shader's CameraBlock declares only `mat4 mvp` and ignores bytes
  64-79; the gizmo shader's CameraBlock reads the full 80. Both unchanged.

## Out of scope

- The 256-byte buffer allocation (`ensureBuffer(..., 256, ...)`) stays - the
  D3D12 256-byte cbuffer alignment requirement is orthogonal to how the first
  80 bytes are written.
- No change to `m_cameraUniformBufferUploaded` flag logic.
