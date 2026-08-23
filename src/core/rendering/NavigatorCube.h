#pragma once

#include <QMatrix4x4>
#include <QPointF>
#include <QVector3D>

// Upstream source truth: GLCanvas3D::_render_3d_navigator
// (third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp:5669-5733) renders
// ImGuizmo::ViewManipulate (third_party/OrcaSlicer/src/imguizmo/ImGuizmo.cpp:
// 2779-3140): a real 3D cube pinned to the viewport bottom-left corner whose
// orientation follows the camera. Clicking a face/edge/corner snaps the view
// with a 40-frame interpolation, dragging on the cube rotates the camera with
// a horizon clamp, and hovered faces highlight. This header ports that math
// 1:1 for the Y-up Qt6 engine (upstream additionally rotates the camera
// matrix 90 degrees around X/Z because ImGuizmo assumes Y-up while the
// upstream scene is Z-up; our scene is already Y-up so no remap is needed).
namespace NavigatorCube
{

// Upstream size = 128 * scale * dpi (GLCanvas3D.cpp:5724); the base size in
// logical pixels.
constexpr float kRectSizePx = 128.0f;
// ImGuizmo.cpp:2817: const float distance = 3.f (cube camera distance).
constexpr float kCubeCameraDistance = 3.0f;
// ImGuizmo.cpp:3078: interpolationFrames = 40.
constexpr int kSnapFrameCount = 40;
// ImGuizmo.cpp:3044: newDir.Lerp(interpolationDir, 0.2f).
constexpr float kSnapDirLerp = 0.2f;
// ImGuizmo.cpp:3090-3091: rotation of 0.01 rad per dragged pixel.
constexpr float kDragRadiansPerPixel = 0.01f;
// Upstream FACE color (dark theme, GLCanvas3D.cpp:5686) and hovered-face
// highlight (ImGuizmo.cpp:2925: IM_COL32(0xF0, 0xA0, 0x60, 0x80)).
constexpr float kFaceColorR = 0.23f;
constexpr float kFaceColorG = 0.23f;
constexpr float kFaceColorB = 0.23f;
constexpr float kFaceColorA = 1.0f;
constexpr float kHoverColorR = 0xF0 / 255.0f;
constexpr float kHoverColorG = 0xA0 / 255.0f;
constexpr float kHoverColorB = 0x60 / 255.0f;
constexpr float kHoverColorA = 0x80 / 255.0f;
// Upstream background quad color 0x00101010 (GLCanvas3D.cpp:5725).
constexpr float kBackgroundR = 0x10 / 255.0f;
constexpr float kBackgroundG = 0x10 / 255.0f;
constexpr float kBackgroundB = 0x10 / 255.0f;
constexpr float kBackgroundA = 0x00 / 255.0f;
// Upstream axis colors map net to the scene axes: X red, Y green, Z blue
// (ColorRGBA::X/Y/Z, libslic3r Color.hpp:143-145; the ImGuizmo DIRECTION
// slot shuffling in GLCanvas3D.cpp:5676-5678 only exists to feed the Z-up
// upstream scene into the Y-up ImGuizmo).
constexpr float kAxisColorX[3] = {0.75f, 0.0f, 0.0f};
constexpr float kAxisColorY[3] = {0.0f, 0.75f, 0.0f};
constexpr float kAxisColorZ[3] = {0.0f, 0.0f, 0.75f};

struct RectF
{
  float x = 0.0f;
  float y = 0.0f;
  float w = kRectSizePx;
  float h = kRectSizePx;
};

// Eye axes expressed in world coordinates, extracted from a world->eye view
// matrix (the camera looks along -Z in eye space).
struct CameraBasis
{
  QVector3D forward;
  QVector3D up;
  QVector3D right;
};

struct Hit
{
  // ImGuizmo overBox: 3x3x3 grid index 0..26 encoding the quantized hit point
  // (cx = box/9, cy = (box % 9)/3, cz = box % 3).
  int box = -1;
  // Face plane that was actually hit (for the hover highlight quad).
  QVector3D faceNormal;
  bool isValid() const { return box >= 0; }
};

CameraBasis cameraBasis(const QMatrix4x4 &view);

// Project a cube-space point (cube spans [-0.5, 0.5]^3) into navigator-rect
// pixels (item coordinates, y down), using the orthographic cube camera of
// ImGuizmo.cpp:2812-2822 (LookAt(camDir * 3, origin, camUp), ortho [-1,1]).
QPointF projectToRect(const QVector3D &cubePos, const CameraBasis &basis,
                      const RectF &rect);

// Parallel-ray hit test against the visible cube faces with the 3x3 panel
// quantization per face (ImGuizmo.cpp:2854-2922 semantics: ray-plane
// intersection with back-face culling, nearest face wins, hit point
// quantized into the 27-box grid).
Hit hitTest(const QPointF &pixel, const CameraBasis &basis, const RectF &rect);

// overBox -> snap direction: (1-cx, 1-cy, 1-cz) normalized
// (ImGuizmo.cpp:3070-3075). The 26 nonzero grid cells cover the 6 face + 12
// edge + 8 corner directions.
QVector3D snapDirectionForBox(int box);

// Drag rotation: rotate dir around world up by dx and around the camera right
// axis by dy (radians, signs per ImGuizmo.cpp:3090-3096), then clamp so the
// direction stays in the world-up hemisphere (horizon clamp,
// ImGuizmo.cpp:3097-3100).
QVector3D dragRotateClamped(const QVector3D &dir, const QVector3D &camRight,
                            float dxRadians, float dyRadians);

// Direction -> CameraController azimuth/elevation in degrees. Vertical
// directions clamp to +-89 degrees because the Qt6 camera builds its view
// matrix with a fixed world-up lookAt (upstream writes a quaternion and has
// no degeneracy at the poles).
void orientationForDirection(const QVector3D &dir, float &azimuthDeg,
                             float &elevationDeg);

} // namespace NavigatorCube
