#pragma once
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include "CameraController.h"

class GLViewport;

/**
 * GLViewportRenderer — OpenGL renderer attached to a GLViewport.
 *
 * Draws:
 *   • Grey build-plate grid  (XZ plane, −100 … +100, step 20)
 *   • RGB axis lines         (X=red, Y=green, Z=blue)
 *
 * Camera input (orbit / pan / zoom) is consumed via synchronize().
 */
class GLViewportRenderer : public QQuickFramebufferObject::Renderer
{
public:
  GLViewportRenderer();
  ~GLViewportRenderer() override;

protected:
  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
  void synchronize(QQuickFramebufferObject *item) override;
  void render() override;

private:
  void initialize();

  bool m_initialized = false;
  QOpenGLFunctions *m_f = nullptr;
  QOpenGLShaderProgram m_prog;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
  int m_totalVertices = 0; // axis: 0-5, grid: 6…
  int m_uMVP = -1;
  int m_uColor = -1;

  CameraController m_camera;
  QSize m_viewSize;

  // drag tracking (GUI-thread state copied in synchronize)
  bool m_dragging = false;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  float m_lastX = 0.f;
  float m_lastY = 0.f;
};
