---
status: resolved
trigger: 单盘鼠标操作还有问题，有些角度无法渲染到。能不能鼠标操作完全模拟一下找到缺失
created: 2026-08-23
updated: 2026-08-23
---

# Symptoms

- Expected: On a single-plate Prepare viewport, mouse orbit/pan/zoom should reach
  and render every camera orientation (full sphere of view directions),
  matching upstream OrcaSlicer `Camera` behavior (no unreachable angles).
- Actual: After mouse-drag orbits to below-horizon camera angles (camera under
  the plate looking up), the viewport renders a uniform gray — model, grid and
  background all invisible. The user described it as "有些角度无法渲染到".
- Error messages: none; no crash.
- Timeline: Persists after the mouse-crash fix (3c116de) and v5.16 multi-plate work.
  Prior smoke sweeps (`_mouse_smoke.ps1`) only did shallow sinusoidal orbit drags —
  below-horizon angles were never exercised.
- Reproduction: Prepare page, single plate, load a model, drag the camera below
  the plate (elevation < 0). Viewport becomes uniform gray.

# Current Focus

- hypothesis: CONFIRMED — Qt6 bed drawing lacks the upstream "bottom" gate.
  The opaque bed fill quad (m_fillPipeline, depth-write ON) is drawn before the
  model mesh; from below-horizon angles the quad sits between camera and model,
  the mesh fails the depth test, and the whole viewport shows the quad's flat
  underside gray (measured 494951/4A4A52, only axis-indicator pixels differ).
- Upstream truth:
  - `GLCanvas3D.cpp:1912/1922`: `_render_bed(..., !camera.is_looking_downward(), ...)`
    and `_render_platelist(..., !is_looking_downward(), ...)` for BOTH View3D and
    CanvasPreview.
  - `PartPlate::render` (PartPlate.cpp:2728-2765): `if (!bottom)` skips
    render_background (plate fill + exclude areas) and render_logo;
    render_grid(bottom) keeps ONLY the grid, recolored LINE_BOTTOM_COLOR
    {0.8,0.8,0.8,0.4} (PartPlate.cpp:85); border + origin axes live in
    render_background → top-only; render_height_limit NOT gated.
  - `Bed3D::render_system` (3DBed.cpp): `if (!bottom) render_model(...)` — bed
    model skipped from below.
  - `Camera::is_looking_downward()` (Camera.hpp:152): forward·worldUp < 0,
    strict — a horizon-level camera counts as NOT looking down (bed hidden).
- test: `_orbit_sweep.ps1` full-sphere mouse harness (azimuth 0-360 × elevation
  +89..-89 ladder + pan extremes + zoom) with per-angle screenshots and
  automatic blank detection (background fraction, distinct colors, frame diff).
- expecting: after the fix, every ladder rung renders the model; below-horizon
  rungs show model + LINE_BOTTOM_COLOR grid instead of uniform gray.

# Evidence

- 2026-08-23: `_orbit_sweep.ps1` on PID 31052 (20mm cube, single plate,
  default settings: freeCamera=false, cameraNavStyle=0; registry HKCU\Software\OWzx
  has no camera overrides):
  - elevation +89/+60/+30 (looking down): renders normally
    (bgFrac 0.74-0.89, distinct colors 21-47, azimuth frame diffs 0.14-0.18).
  - elevation -40: uniform gray — bgFrac 0.0000, distinct 15-38, lumStd 3.6-5.4,
    dominant colors 494951/4A4A52 (bed fill underside), azimuth diffs 0.035-0.061.
  - elevation -89: uniform gray — distinct 11-32, lumStd 2.4-3.3, azimuth
    diffs 0.007-0.009 (visually frozen).
  - pan + zoom phases: fine (camera moves; diff 0.2-0.25).
- 2026-08-23: Qt6 draw path (RhiViewportRenderer.cpp render()): bed fill drawn
  FIRST with m_fillPipeline (opaque, depth-write ON, created via ensurePipelines
  default enableDepthWrite=true), model mesh drawn AFTER with depth test —
  from below the fill quad occludes the mesh.
- 2026-08-23: upstream `PartPlate::render` + `Bed3D::render_system` verified to
  skip plate background/exclude/logo/bed-model when bottom=true, keeping only
  render_grid in LINE_BOTTOM_COLOR; `_render_bed`/`_render_platelist` pass
  bottom=!is_looking_downward() in both View3D and CanvasPreview paths.

# Eliminated

- hypothesis: NaN view matrix at elevation ±90 (lookAt up-vector degeneracy).
  reason: default freeCamera=false clamps elevation to ±89; observed failure
  starts at ~-40, far from the singularity; the rendered uniform quad proves
  the view matrix is valid.
- hypothesis: QML overlay MouseAreas stealing drags at some angles.
  reason: azimuth frame diffs prove orbit events are delivered at every rung;
  the viewport content itself is a valid render of the bed underside.
- hypothesis: free camera / cameraNavStyle user setting involvement.
  reason: HKCU\Software\OWzx registry holds no camera keys; defaults used.

# Resolution

- root_cause: Qt6 bed rendering omitted the upstream below-horizon ("bottom")
  gate: from below the plate the opaque bed fill (and texture/model) occluded
  the entire scene, so those camera angles appeared unrenderable.
- fix: mirrored the upstream gate —
  1. RhiViewportRenderer::cameraLookingDown() (m_cameraView⁻¹ · eye -Z; y<0,
     matching upstream is_looking_downward's strict < 0),
  2. View3D + CanvasPreview bed blocks skip fill/lines/texture/bed-model when
     not looking down,
  3. PrepareSceneData bakes a grid-only bedBottomLineVertices in
     LINE_BOTTOM_COLOR {0.8,0.8,0.8,0.4} drawn via m_translucentLinePipeline
     (border/origin axes stay top-only, matching render_background gating;
     height-limit rings stay ungated like upstream),
  4. regressions: PrepareSceneDataTests::bedBottomLineGridUsesUpstreamBottomColor,
     QmlUiAuditTests::rhiViewportRendererGatesBedSurfacesOnLookingDown.
- verification: canonical verifier (scripts/auto_verify_with_vcvars.ps1) exit 0,
  all suites green (PrepareScene 18, PartPlate 60, ObjectPicking 7,
  ViewModelSmoke 162, QmlUiAudit 154, ViewportContext, PreviewParser, E2E).
  Full-sphere mouse harness rerun on the fixed build (PID 34864, same cube):
  every elevation rung from +89 to -89 now renders (lumStd 26-67 vs 2-5
  before), azimuth drags change the frame at every rung (diff 0.08-0.24 vs
  0.007-0.06), below-horizon frames show the model (E4E4E4) plus the blended
  bottom grid (858689) on the background, pan/zoom phases stable, no crash.
- build-landmine note (pre-existing, NOT caused by this fix but triggered by
  it): the build tree has no header dependency tracking (ninja -t deps shows
  "#deps 0" for every object — localized MSVC /showIncludes output is not
  parsed), so changing a class layout (PrepareSceneData gained a member) does
  not recompile dependent TUs. ObjectPickingTests crashed (0xc0000005 in the
  first test) due to stale-object ABI mismatch; fixed by deleting all stale
  .obj files whose sources include PrepareSceneData.h / RhiViewport.h /
  EditorViewModel.h / SoftwareViewport.h and rebuilding. If class layouts
  change again, expect the same failure mode.
- files_changed: src/qml_gui/Renderer/PrepareSceneData.h/.cpp,
  src/qml_gui/Renderer/RhiViewportRenderer.h/.cpp,
  tests/PrepareSceneDataTests.cpp, tests/QmlUiAuditTests.cpp.
