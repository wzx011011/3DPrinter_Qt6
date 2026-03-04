#pragma once
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QByteArray>
#include "CameraController.h"

class GLViewport;

/**
 * GLViewportRenderer — OpenGL renderer attached to a GLViewport.
 *
 * Draws:
 *   • Grey build-plate grid  (XZ plane, −100 … +100, step 20)
 *   • RGB axis lines         (X=red, Y=green, Z=blue)
 *   • Loaded model mesh      (flat-shaded triangles, uploaded after loadFile)
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
  void uploadMesh();

  bool m_initialized = false;
  QOpenGLFunctions *m_f = nullptr;

  // ─── 基础着色器 (grid + axes) ───────────────────────────────────────────
  QOpenGLShaderProgram m_prog;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
  int m_totalVertices = 0;
  int m_uMVP = -1;
  int m_uColor = -1;

  // ─── 网格着色器 (flat-shaded triangles) ─────────────────────────────────
  QOpenGLShaderProgram m_meshProg;
  QOpenGLVertexArrayObject m_meshVao;
  QOpenGLBuffer m_meshVbo{QOpenGLBuffer::VertexBuffer};
  int m_meshVertexCount = 0;   // 渲染帧用的顶点数
  int m_uMeshMVP = -1;

  // GUI 线程推送过来的待上传数据
  QByteArray m_pendingMesh;
  bool m_meshDirty = false;
  int m_meshVersion = -1;      // 与 GLViewport::m_meshVersion 同步

  CameraController m_camera;
  QSize m_viewSize;

  // drag tracking (GUI-thread state copied in synchronize)
  bool m_dragging = false;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  float m_lastX = 0.f;
  float m_lastY = 0.f;
};
