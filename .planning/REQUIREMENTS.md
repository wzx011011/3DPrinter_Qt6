# Requirements: OWzx Slicer v3.8 RHI Gizmo Parity

**Defined:** 2026-07-04
**Status:** Active - requirements defined, roadmap ready
**Core Value:** The default D3D11 RHI rendering path must be functionally
complete for gizmo-based object manipulation, so that the GLViewport
(OpenGL fallback) can be retired and the RHI path is the sole functional
render path.

## Scope Contract

v3.8 ports the gizmo system (move/rotate/scale/cut), wipe tower, cut plane,
and precise object picking from `GLViewportRenderer` (2285 lines, OpenGL path
gated by `OWZX_OPENGL=1`) to `RhiViewportRenderer` (the default D3D11 path).
On completion, GLViewportRenderer is deleted and the GL path no longer exists.

The default D3D11 path currently renders the bed grid (fixed in this session)
but has ZERO gizmo support — clicking Move/Rotate/Scale buttons in the
vertical toolbar sets gizmoMode but nothing renders and drag does nothing
(silent dead UI). This is the single largest functional gap in the default
path.

Carry-forward constraints:
- D3D12 segfault (QRhi backend deep issue) — D3D11 stays default throughout v3.8.
- v3.6 VERIFY-04 manual visual UAT still pending (independent of this milestone).

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, credential, protocol, product decision, or upstream feature that is not locally available.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.
- **Superseded:** previous-milestone scope is intentionally abandoned in favor of this milestone.

## v3.8 Requirements

### Gizmo Math Foundation

- [ ] **GMATH-01:** Pure-math gizmo functions (computeRay, rayToAxisT, computeRotateAngle, rayXZIntersect) are extracted into a dependency-free library with unit tests covering each function.
- [ ] **GMATH-02:** Axis-pick functions (pickMoveAxis, pickRotateAxis, pickScaleAxis) are extracted, parameterized (no member-variable coupling), and unit-tested.
- [ ] **GMATH-03:** GLViewportRenderer continues to work identically after refactoring to call the extracted GizmoMath library (equivalence verified by behavior).

### Gizmo Geometry

- [ ] **GGEO-01:** Move gizmo geometry (3 axis arrows: shaft lines + cone triangles) is generated as pure CPU vertex data, decoupled from any GL/RHI calls.
- [ ] **GGEO-02:** Rotate gizmo geometry (3 torus rings) and scale gizmo geometry (3 axis shafts + box handles) are generated as pure CPU vertex data.
- [ ] **GGEO-03:** Geometry generators produce vertex counts identical to the GL originals; snapshot tests pin the layout.

### RHI State Wiring

- [ ] **GWIRE-01:** RhiViewportRenderer::synchronize reads gizmoMode, cutAxis, cutPosition, and computes gizmoCenter from the selected object's AABB. The broken state pipeline is connected.
- [ ] **GWIRE-02:** RhiViewport::setGizmoMode (and cutAxis/cutPosition setters) call update() so the renderer re-syncs on state change.

### Move Gizmo

- [ ] **GMOV-01:** Setting gizmoMode to Move renders three colored axis arrows (X red, Y green, Z blue) at the selected object's position on the default D3D11 path.
- [ ] **GMOV-02:** The gizmo renders on top of geometry (not occluded by the model) using depth-clear or no-depth-write.
- [ ] **GMOV-03:** Clicking a gizmo axis selects that axis for dragging (pickMoveAxis via GizmoMath).
- [ ] **GMOV-04:** Dragging the selected axis moves the object along that axis only; camera orbit is not triggered during gizmo drag.

### Rotate + Scale Gizmos

- [ ] **GROT-01:** Rotate gizmo renders 3 torus rings; dragging a ring rotates the object around that axis.
- [ ] **GROT-02:** Rotate pick precision matches the GL path (ray-plane + ring radius test).
- [ ] **GSCA-01:** Scale gizmo renders 3 axis shafts with box handles; dragging a handle scales the object along that axis.
- [ ] **GSCA-02:** Scale pick precision matches the GL path.

### Cut Plane + Wipe Tower

- [ ] **GCUT-01:** Cut plane renders as a translucent quad with outline; cutAxis and cutPosition changes update the plane in real time.
- [ ] **GWT-01:** Wipe tower renders as a translucent box when present in the scene; synchronize reads the wipe tower properties (currently broken, same as gizmo state).

### Precise Object Picking

- [ ] **GPICK-01:** Object picking on the default path uses ray-triangle (Möller-Trumbore) precision matching the GL path, replacing the current AABB-screen-rectangle approximation.

### GLViewport Retirement

- [ ] **GRET-01:** GLViewportRenderer.{cpp,h}, GLViewport.{cpp,h}, and GCodeRenderer's GLViewport dependency are removed; the build succeeds without them (~2285 lines deleted).
- [ ] **GRET-02:** The OWZX_OPENGL environment flag no longer has any effect (no GL path to activate); only RhiViewport + SoftwareViewport remain in the registration.

## Future Requirements

- D3D12 default backend promotion (blocked on QRhi D3D12 segfault root-cause).
- Device/cloud/Monitor workflows (separate milestone).
- Hardware calibration completion (separate milestone).

## Out of Scope

| Feature | Reason |
|---|---|
| D3D12 backend as default | D3D12 segfault at setShaderResources is a QRhi backend deep issue; needs QRhi internal fix or Qt upgrade. D3D11 stays default. |
| New gizmo types (measure, flatten, support/seam paint) | These depend on OpenVDB (unavailable) or are out of the OrcaSlicer v7.0.1 gizmo parity scope. Future work. |
| Material editor / PBR lighting | QtQuick3D migration territory (out of scope per Qt评估文档). |
| Editor features beyond gizmo parity | v3.8 is purely about making the default RHI path functionally complete for the existing gizmo/pick/cut/wipe feature set. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| GMATH-01 | Phase 65 | Pending |
| GMATH-02 | Phase 65 | Pending |
| GMATH-03 | Phase 65 | Pending |
| GGEO-01 | Phase 66 | Pending |
| GGEO-02 | Phase 66 | Pending |
| GGEO-03 | Phase 66 | Pending |
| GWIRE-01 | Phase 67 | Pending |
| GWIRE-02 | Phase 67 | Pending |
| GMOV-01 | Phase 68 | Pending |
| GMOV-02 | Phase 68 | Pending |
| GMOV-03 | Phase 69 | Pending |
| GMOV-04 | Phase 69 | Pending |
| GROT-01 | Phase 70 | Pending |
| GROT-02 | Phase 70 | Pending |
| GSCA-01 | Phase 70 | Pending |
| GSCA-02 | Phase 70 | Pending |
| GCUT-01 | Phase 71 | Pending |
| GWT-01 | Phase 71 | Pending |
| GPICK-01 | Phase 72 | Pending |
| GRET-01 | Phase 73 | Pending |
| GRET-02 | Phase 73 | Pending |

**Coverage:** 21 total; 21 mapped; 0 unmapped.

---

*Requirements defined: 2026-07-04*
*Last updated: 2026-07-04 for v3.8 milestone definition.*
