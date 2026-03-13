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
// Left drag (gizmo)   : depends on gizmo mode (move/rotate/scale)
// Left drag (object)  : free XZ-plane move
// Left drag (empty)   : orbit camera
// Middle drag         : pan  |  Scroll : zoom
class GLViewportRenderer : public QQuickFramebufferObject::Renderer
{
public:
  enum GizmoMode { GizmoMove = 0, GizmoRotate = 1, GizmoScale = 2 };

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
  void buildRotateGizmoGeometry();
  void buildScaleGizmoGeometry();
  void renderMoveGizmo(const QMatrix4x4 &mvp);
  void renderRotateGizmo(const QMatrix4x4 &mvp);
  void renderScaleGizmo(const QMatrix4x4 &mvp);
  void renderGizmo(const QMatrix4x4 &mvp);

  std::pair<QVector3D, QVector3D> computeRay(float sx, float sy) const;
  QVector3D rayXZIntersect(float sx, float sy) const;
  int pickObject(float sx, float sy) const;
  int pickMoveAxis(float sx, float sy) const;   // 0=none 1=X 2=Y 3=Z
  int pickRotateAxis(float sx, float sy) const; // 0=none 1=X 2=Y 3=Z
  int pickScaleAxis(float sx, float sy) const;  // 0=none 1=X 2=Y 3=Z 4=uniform
  float rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                   const QVector3D &axisDir) const;
  float computeRotateAngle(float sx, float sy, int axis) const;

  // Compute model matrix from per-object transform
  QMatrix4x4 computeModelMatrix(int objectId) const;
  // Get transformed AABB for picking
  void transformedAABB(int objectId, float outMin[3], float outMax[3]) const;

  bool m_initialized = false;
  QOpenGLExtraFunctions *m_f = nullptr;

  // glPolygonMode resolved at init (not in QOpenGLExtraFunctions on Qt 6.10)
  using GlPolygonModeFn = void (*)(GLenum, GLenum);
  GlPolygonModeFn m_glPolygonMode = nullptr;

  // Base shader (grid + axes)
  QOpenGLShaderProgram m_prog;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
  int m_axisStart = 0, m_axisCount = 0;
  int m_borderStart = 0, m_borderCount = 0;
  int m_fineGridStart = 0, m_fineGridCount = 0;
  int m_coarseGridStart = 0, m_coarseGridCount = 0;
  int m_uMVP = -1, m_uColor = -1;

  // Mesh shader (full transform via mat4)
  QOpenGLShaderProgram m_meshProg;
  int m_uMeshMVP = -1, m_uMeshModel = -1, m_uMeshBaseColor = -1, m_uMeshBright = -1;

  // Gizmo shader (local verts shifted by uCenter and scaled by uGizmoScale)
  QOpenGLShaderProgram m_gizmoProg;
  int m_uGizmoMVP = -1, m_uGizmoCenter = -1, m_uGizmoScale = -1, m_uGizmoColor = -1;

  // Move gizmo (arrow geometry)
  GLuint m_moveGizmoVao = 0, m_moveGizmoVbo = 0;
  static constexpr int kConeSides = 6;
  int m_moveShaftBase[3] = {0, 0, 0};
  int m_moveConeBase[3] = {0, 0, 0};

  // Rotate gizmo (torus ring geometry, 3 rings)
  GLuint m_rotateGizmoVao = 0, m_rotateGizmoVbo = 0;
  int m_rotateRingBase[3] = {0, 0, 0}; // vertex offset for each ring
  int m_rotateRingVertCount = 0;        // vertices per ring

  // Scale gizmo (shaft + box geometry)
  GLuint m_scaleGizmoVao = 0, m_scaleGizmoVbo = 0;
  int m_scaleShaftBase[3] = {0, 0, 0}; // shaft line start per axis
  int m_scaleBoxBase[3] = {0, 0, 0};   // box geometry start per axis
  int m_scaleBoxVertCount = 0;          // vertices per box

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

  // Gizmo mode
  GizmoMode m_gizmoMode = GizmoMove;

  // Wireframe mode
  bool m_wireframeMode = false;

  // Mouse state
  bool m_mouseDragging = false;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  float m_lastX = 0.f, m_lastY = 0.f;

  // Selection
  int m_selectedId = -1;

  // Free XZ object drag
  bool m_freeXZDragging = false;
  QVector3D m_freeXZOrigin;

  // Gizmo axis drag (1=X 2=Y 3=Z 4=uniform, 0=inactive)
  int m_gizmoAxis = 0;
  float m_gizmoAxisT = 0.f;
  QVector3D m_gizmoCenter;

  // Rotate drag state
  float m_rotateStartAngle = 0.f; // initial angle for rotation drag

  // Per-object transform data
  struct ObjectTransform
  {
    QVector3D translation{0, 0, 0};
    QVector3D rotation{0, 0, 0}; // Euler angles in degrees
    QVector3D scale{1, 1, 1};
  };

  // Per-drag tracking for transform history
  bool m_objectDragActive = false;
  int m_dragObjectId = -1;
  ObjectTransform m_dragStartTransform;

  struct TransformCmd
  {
    int objectId = -1;
    ObjectTransform before;
    ObjectTransform after;
  };
  std::vector<TransformCmd> m_undoStack;
  std::vector<TransformCmd> m_redoStack;

  // Per-object transforms
  std::unordered_map<int, ObjectTransform> m_objectTransforms;
};
