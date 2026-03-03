#include "CameraController.h"
#include <QtMath>

void CameraController::orbit(float dAzimuth, float dElevation)
{
  m_azimuth += dAzimuth;
  m_elevation = qBound(-89.0f, m_elevation + dElevation, 89.0f);
}

void CameraController::pan(float dx, float dy)
{
  const float az = qDegreesToRadians(m_azimuth);
  const float el = qDegreesToRadians(m_elevation);

  // Forward vector (from target toward eye)
  QVector3D forward(
      qCos(el) * qSin(az),
      qSin(el),
      qCos(el) * qCos(az));

  QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
  QVector3D up = QVector3D::crossProduct(right, forward).normalized();

  const float scale = m_distance * 0.001f;
  m_target -= right * (dx * scale);
  m_target += up * (dy * scale);
}

void CameraController::zoom(float delta)
{
  m_distance = qMax(10.0f, m_distance - delta * 0.5f);
}

QMatrix4x4 CameraController::viewMatrix() const
{
  const float az = qDegreesToRadians(m_azimuth);
  const float el = qDegreesToRadians(m_elevation);

  QVector3D dir(
      qCos(el) * qSin(az),
      qSin(el),
      qCos(el) * qCos(az));

  const QVector3D eye = m_target + dir * m_distance;
  QMatrix4x4 mat;
  mat.lookAt(eye, m_target, QVector3D(0, 1, 0));
  return mat;
}

QMatrix4x4 CameraController::projMatrix(float aspect) const
{
  QMatrix4x4 mat;
  mat.perspective(45.0f, aspect, 1.0f, 10000.0f);
  return mat;
}
