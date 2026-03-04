#pragma once
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLExtraFunctions>
#include <QByteArray>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <unordered_map>
#include <utility>
#include "CameraController.h"

class GLViewport;

// GLViewportRenderer - OpenGL renderer attached to a GLViewport.
// Left click          : pick object / gizmo axis
// Left drag (gizmo)   : move along X / Y / Z axis (constrained)
// Left drag (object)  : free XZ-plane move
// Left drag (empty)   : orbit camera
// Middle drag         : pan  |  Scroll : zoom
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
  void buildGizmoGeometry();
  void renderGizmo(const QMatrix4x4 &mvp);

  std::pair<QVector3D, QVector3D> computeRay(float sx, float sy) const;
  QVector3D rayXZIntersect(float sx, float sy) const;
  int pickObject(float sx, float sy) const;
  int pickGizmoAxis(float sx, float sy) const; // 0=none 1=X 2=Y 3=Z
  float rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                   const QVector3D &axisDir) const;

  bool m_initialized = false;
  QOpenGLExtraFunctions *m_f = nullptr;

  // Base shader (grid + axes)
  QOpenGLShaderProgram m_prog;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
  int m_axisStart = 0, m_axisCount = 0;
  int m_borderStart = 0, m_borderCount = 0;
  int m_fineGridStart = 0, m_fineGridCount = 0;
  int m_coarseGridStart = 0, m_coarseGridCount = 0;
  int m_uMVP = -1, m_uColor = -1;

  // Mesh shader
  QOpenGLShaderProgram m_meshProg;
  int m_uMeshMVP = -1, m_uMeshBaseColor = -1, m_uMeshOffset = -1, m_uMeshBright = -1;

  // Gizmo shader
  QOpenGLShaderProgram m_gizmoProg;
  int m_uGizmoMVP = -1, m_uGizmoCenter = -1, m_uGizmoScale = -1, m_uGizmoColor = -1;
  GLuint m_gizmoVao = 0, m_gizmoVbo = 0;
  static constexpr int kConeSides = 6;
  int m_gizmoShaftBase[3] = {0, 0, 0};
  int m_gizmoConeBase[3] = {0, 0, 0};

  // Mesh batches
  struct MeshBatch
  {
    GLuint vao = 0, vbo = 0;
    int vertexCount = 0, objectId = 0;
    float bboxMin[3] = {0, 0, 0}, bboxMax[3] = {0, 0, 0};
  };
  std::vector<MeshBatch> m_meshBatches;
  QByteArray m_pendingMesh;
  bool m_meshDirty = false;
  int m_meshVersion = -1;

  CameraController m_camera;
  QSize m_viewSize;

  // Mouse state
  bool m_mouseDragging = false;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  float m_lastX = 0.f, m_lastY = 0.f;

  // Selection
  int m_selectedId = -1;

  // Free XZ object drag
  bool m_freeXZDragging = false;
  QVector3D m_freeXZOrigin;

  // Gizmo axis drag (1=X 2=Y 3=Z, 0=inactive)
  int m_gizmoAxis = 0;
  float m_gizmoAxisT = 0.f;
  QVector3D m_gizmoCenter;

  // Accumulated per-object translation offsets
  std::unordered_map<int, QVector3D> m_objectOffsets;
};