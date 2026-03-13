#pragma once
#include <QMatrix4x4>
#include <QVector3D>

/**
 * Orbit camera:
 *   - Left drag  → orbit (azimuth / elevation)
 *   - Middle drag → pan  (target translation)
 *   - Wheel       → zoom (distance)
 */
class CameraController
{
public:
  void orbit(float dAzimuth, float dElevation);
  void pan(float dx, float dy);
  void zoom(float delta);

  /// 加载模型后自适应视角: 将相机对准 bbox 球心，距离自动适配
  void fitView(float cx, float cy, float cz, float radius);
  /// 重置到 K1C 平台默认视角
  void resetToDefault();

  /// Camera presets (matching upstream GCodeViewer view presets)
  void viewTop();
  void viewFront();
  void viewRight();
  void viewIso();

  QMatrix4x4 viewMatrix() const;
  QMatrix4x4 projMatrix(float aspect) const;
  QVector3D  eye() const;         // world-space camera position
  QVector3D  target() const;      // orbit target (look-at point)
  float      distance() const;    // camera distance from target

private:
  float m_azimuth = 45.0f;    // degrees, horizontal
  float m_elevation = 35.0f;  // degrees, vertical
  float m_distance = 380.0f;  // world units
  // 默认 target = K1C 打印平台中心 (GL: x=110, y=0, z=110)
  QVector3D m_target{110.f, 0.f, 110.f};
};
