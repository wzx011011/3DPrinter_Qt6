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

  QMatrix4x4 viewMatrix() const;
  QMatrix4x4 projMatrix(float aspect) const;

private:
  float m_azimuth = 45.0f;   // degrees, horizontal
  float m_elevation = 30.0f; // degrees, vertical
  float m_distance = 350.0f; // world units
  QVector3D m_target{0.f, 0.f, 0.f};
};
