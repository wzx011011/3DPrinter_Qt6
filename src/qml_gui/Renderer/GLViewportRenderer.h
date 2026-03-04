#pragma once
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLExtraFunctions>
#include <QByteArray>
#include <QVector3D>
#include <vector>
#include "CameraController.h"

class GLViewport;

/**
 * GLViewportRenderer — OpenGL renderer attached to a GLViewport.
 *
 * Draws:
 *   • 220×220mm build plate grid (10mm fine / 50mm coarse / white border)
 *   • RGB axis lines              (X=red, Y=green, Z=blue)
 *   • Per-object mesh batches     (flat-shaded, distinct colours)
 *
 * Camera input (orbit / pan / zoom / fitView) from synchronize().
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
  QOpenGLExtraFunctions *m_f = nullptr;

  // ─── 基础着色器 (grid + axes) ───────────────────────────────────────────
  QOpenGLShaderProgram m_prog;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};

  // 各段顶点偏移／数量
  int m_axisStart       = 0;
  int m_axisCount       = 0;
  int m_borderStart     = 0;
  int m_borderCount     = 0;
  int m_fineGridStart   = 0;
  int m_fineGridCount   = 0;
  int m_coarseGridStart = 0;
  int m_coarseGridCount = 0;

  int m_uMVP   = -1;
  int m_uColor = -1;

  // ─── 多对象 Mesh 着色器 (flat-shaded per-object colour) ─────────────────
  QOpenGLShaderProgram m_meshProg;
  int m_uMeshMVP       = -1;
  int m_uMeshBaseColor = -1;

  // Raw GL handles (Qt wrappers are non-copyable/non-movable, can't live in vector)
  struct MeshBatch
  {
    GLuint vao        = 0;
    GLuint vbo        = 0;
    int    vertexCount = 0;
    int    objectId    = 0;
  };
  std::vector<MeshBatch> m_meshBatches;

  // GUI 线程推送过来的待上传数据
  QByteArray m_pendingMesh;
  bool m_meshDirty = false;
  int  m_meshVersion = -1;

  CameraController m_camera;
  QSize m_viewSize;

  // drag tracking
  bool m_dragging = false;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  float m_lastX = 0.f;
  float m_lastY = 0.f;
};
