#pragma once
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>

/**
 * Orbit camera mirroring upstream Slic3r::GUI::Camera (Camera.cpp):
 *   - Constrained mode: rotate_on_sphere semantics (Camera.cpp:373-392) --
 *     world-up yaw + camera-right pitch with the zenit clamped to exactly
 *     +-90 degrees (straight top-down / bottom-up are reachable; the
 *     quaternion orientation has no lookAt pole singularity).
 *   - Free mode: rotate_local_around_target virtual trackball
 *     (Camera.cpp:409-420) -- no angle limits, roll is allowed.
 *   - recover_from_free_camera re-aligns the right vector horizontally
 *     before constrained rotations (Camera.hpp:154-158).
 *   - Wheel zoom is multiplicative with upstream bounds
 *     [0.2 * fit factor, max_zoom=250] mapped to a distance range
 *     (Camera.cpp:75-84, Camera.hpp:165-166).
 *   - Target mutations clamp into the scene box enlarged 3x
 *     (Camera.cpp:662-674 validate_target).
 */
class CameraController
{
public:
  CameraController();

  // v5.12: freeCamera enables the upstream "use free camera" virtual
  // trackball (Preferences.cpp:1127); constrained mode is the default.
  void setFreeCamera(bool free) { m_freeCamera = free; }
  bool freeCamera() const { return m_freeCamera; }

  // Constrained orbit in degrees. dElevation stops exactly at +-90 (the
  // overshoot is trimmed from the applied pitch, upstream Camera.cpp:376-385).
  void orbit(float dAzimuth, float dElevation);
  // Upstream rotate_on_sphere_with_target (Camera.cpp:351-370): orbit around
  // an arbitrary pivot (selection/volume bbox center); eye and target rotate
  // about the pivot, pivot distance is preserved.
  void orbitAround(float dAzimuth, float dElevation, const QVector3D &pivot);
  // Upstream rotate_local_around_target (Camera.cpp:409-420): virtual
  // trackball around camera-local axes; angles in radians, no limits.
  void rotateLocalAroundTarget(float pitchAroundRightRad, float yawAroundUpRad);
  // Upstream Camera::recover_from_free_camera (Camera.hpp:154-158): forces
  // the right vector parallel to the horizon, keeping eye/target/distance.
  void recoverFromFreeCamera();

  // Pick-based pan: upstream pans by unprojecting the cursor onto the bed
  // plane and translating the target by the world delta
  // (GLCanvas3D.cpp:4353-4359 set_target(target + orig - cur_pos)).
  void translateWorld(const QVector3D &displacement);

  // Upstream update_zoom delta semantics (Camera.cpp:83 update_zoom:
  // m_zoom /= 1 - clamp(delta, +-4) * ZoomUnit(0.1), GLCanvas3D.cpp:3765
  // wheel delta is +-1 per notch) mapped onto distance (zoom ~ 1/distance),
  // bounded by the scene box (Camera.cpp:75-84 set_zoom).
  void zoom(float delta);

  // v5.16 (NAVIGATOR): absolute orientation write-back for the 3D navigator
  // cube (upstream camera.set_rotation preserves target/distance). Elevation
  // clamps to +-90 exactly; the quaternion orientation keeps the view
  // well-defined at the poles.
  void setOrientation(float azimuthDeg, float elevationDeg);

  /// 加载模型后自适应视角: 将相机对准 bbox 球心，距离自动适配
  void fitView(float cx, float cy, float cz, float radius);
  /// 重置到 K1C 平台默认视角
  void resetToDefault();

  /// Camera presets (matching upstream GCodeViewer view presets)
  void viewTop();
  void viewFront();
  void viewRight();
  void viewIso();
  // Upstream Camera::select_view("plate") / "topfront": oblique bed view.
  void viewPlate();
  // Phase 237 (VIEW-01): remaining upstream Camera::select_view directions
  // (Camera.cpp:86-107). Azimuth/elevation table derived from the established
  // Qt6 mapping (Y-up, azimuth measured from +Z toward +X, front=+Z,
  // right=+X, established by viewFront/viewRight above).
  void viewBottom();
  void viewRear();
  void viewLeft();

  // Phase 237 (VIEW-01): perspective/orthographic projection toggle.
  // Upstream Camera::EType { Perspective, Ortho } switched by the View menu
  // radio items "Use Perspective View" / "Use Orthogonal View"
  // (MainFrame.cpp:2604-2620, app_config use_perspective_camera).
  void setUseOrtho(bool ortho) { m_useOrtho = ortho; }
  bool useOrtho() const { return m_useOrtho; }

  // Scene box for zoom bounds and target validation (upstream m_scene_box,
  // Camera::set_scene_box). Extents are GL-world bed footprint + height.
  void setSceneExtent(float width, float depth, float height);

  // Upstream validate_target (Camera.cpp:662-674): target clamped into the
  // scene box enlarged 3x around its center.
  QVector3D clampTarget(const QVector3D &target) const;

  QMatrix4x4 viewMatrix() const;
  QMatrix4x4 projMatrix(float aspect) const;
  QVector3D  eye() const;         // world-space camera position
  QVector3D  target() const;      // orbit target (look-at point)
  float      distance() const;    // camera distance from target
  // Camera basis vectors in world space (upstream get_dir_right/up/forward,
  // Camera.hpp:99-101).
  QVector3D  rightVector() const;
  QVector3D  upVector() const;
  QVector3D  forwardVector() const; // camera look direction (target - eye)
  // v5.16 (NAVIGATOR): current spherical orientation (degrees), derived from
  // the orientation quaternion (azimuth = atan2 of the horizontal
  // target->eye direction, elevation = its vertical angle; 0 at the poles).
  float      azimuth() const;
  float      elevation() const;

  // Screen (item pixel, y-down) -> NDC -> ray hit on the bed plane y=0.
  // Backs the upstream _mouse_to_3d(..., z=0) pick used by pan and
  // zoom-to-mouse (GLCanvas3D.cpp:3772-3783, :4353-4359). Returns false when
  // the ray is parallel to the plane.
  static bool groundPointOnPlane(const QMatrix4x4 &viewProj,
                                 const QSizeF &viewport,
                                 const QPointF &screen, QVector3D *outWorld);

private:
  void clampDistance();
  // Orientation quaternion: rotatedVector maps camera-local (1,0,0)=right,
  // (0,1,0)=up, (0,0,-1)=look direction into world space.
  QQuaternion rotationFromAzEl(float azimuthDeg, float elevationDeg) const;
  void setRotationAzEl(float azimuthDeg, float elevationDeg);

  QQuaternion m_rotation;
  float m_distance = 380.0f;  // world units
  // 默认 target = K1C 打印平台中心 (GL: x=110, y=0, z=110)
  QVector3D m_target{110.f, 0.f, 110.f};
  bool m_freeCamera = false;  // upstream use_free_camera
  bool m_useOrtho = false;    // Phase 237 (VIEW-01): orthographic projection
  // Scene box (GL world). Defaults to the K1C 220x220 bed footprint with a
  // 120 height; RhiViewport refreshes it from the live bed properties.
  float m_sceneMinX = 0.0f, m_sceneMaxX = 220.0f;
  float m_sceneMinY = 0.0f, m_sceneMaxY = 120.0f;
  float m_sceneMinZ = 0.0f, m_sceneMaxZ = 220.0f;
  float m_sceneRadius = 170.0f;
};
