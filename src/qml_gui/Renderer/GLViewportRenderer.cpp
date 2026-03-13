#include "GLViewportRenderer.h"
#include "GLViewport.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <cstring>
#include <cfloat>
#include <algorithm>
#include <cmath>
#include <limits>
#include <exception>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
static const char *kVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "void main(){ gl_Position = uMVP * vec4(aPos,1.0); }\n";

static const char *kFragSrc =
    "#version 330 core\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = uColor; }\n";

static const char *kMeshVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "out vec3 vWorldPos;\n"
    "void main(){\n"
    "  vec4 p = uModel * vec4(aPos, 1.0);\n"
    "  vWorldPos = p.xyz;\n"
    "  gl_Position = uMVP * p;\n"
    "}\n";

static const char *kMeshFragSrc =
    "#version 330 core\n"
  "in  vec3 vWorldPos;\n"
    "out vec4 fragColor;\n"
    "uniform vec3  uBaseColor;\n"
    "uniform float uBrightness;\n"
    "void main(){\n"
  "  vec3 col = mix(uBaseColor, vec3(1.0, 1.0, 1.0), 0.18) * uBrightness;\n"
  "  fragColor = vec4(col, 1.0);\n"
    "}\n";

// Gizmo shader: local verts shifted by uCenter and scaled by uGizmoScale
static const char *kGizmoVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4  uMVP;\n"
    "uniform vec3  uGizmoCenter;\n"
    "uniform float uGizmoScale;\n"
    "void main(){\n"
    "  gl_Position = uMVP * vec4(uGizmoCenter + aPos*uGizmoScale, 1.0);\n"
    "}\n";

static const char *kGizmoFragSrc =
    "#version 330 core\n"
    "uniform vec4 uGizmoColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = uGizmoColor; }\n";

// ---------------------------------------------------------------------------
// Colour palette
// ---------------------------------------------------------------------------
static const QVector3D kObjColors[] = {
    QVector3D(0.949f, 0.549f, 0.220f),
    QVector3D(0.231f, 0.553f, 0.867f),
    QVector3D(0.298f, 0.686f, 0.455f),
    QVector3D(0.608f, 0.349f, 0.714f),
    QVector3D(0.906f, 0.298f, 0.235f),
    QVector3D(0.102f, 0.737f, 0.612f),
    QVector3D(0.945f, 0.769f, 0.059f),
    QVector3D(0.914f, 0.118f, 0.549f),
};
static constexpr int kNumColors = int(sizeof(kObjColors) / sizeof(kObjColors[0]));

// Gizmo axis colors (RGBA), index 0=X 1=Y 2=Z
static const QVector4D kAxisColors[3] = {
    QVector4D(0.90f, 0.18f, 0.18f, 1.f),
    QVector4D(0.22f, 0.80f, 0.22f, 1.f),
    QVector4D(0.18f, 0.40f, 0.95f, 1.f),
};
static const QVector4D kAxisHighlight[3] = {
    QVector4D(1.00f, 0.50f, 0.50f, 1.f),
    QVector4D(0.50f, 1.00f, 0.50f, 1.f),
    QVector4D(0.50f, 0.70f, 1.00f, 1.f),
};

// ---------------------------------------------------------------------------
// Platform geometry
// ---------------------------------------------------------------------------
struct GVertex
{
  float x, y, z;
};

static QVector<GVertex> buildGeometry(
    int &axisStart, int &axisCount,
    int &borderStart, int &borderCount,
    int &fineStart, int &fineCount,
    int &coarseStart, int &coarseCount)
{
  QVector<GVertex> v;
  v.reserve(512);
  axisStart = v.size();
  v.append({0, 0, 0});
  v.append({30, 0, 0});
  v.append({0, 0, 0});
  v.append({0, 30, 0});
  v.append({0, 0, 0});
  v.append({0, 0, 30});
  axisCount = v.size() - axisStart;
  borderStart = v.size();
  const float P = 220.f;
  v.append({0, 0, 0});
  v.append({P, 0, 0});
  v.append({P, 0, 0});
  v.append({P, 0, P});
  v.append({P, 0, P});
  v.append({0, 0, P});
  v.append({0, 0, P});
  v.append({0, 0, 0});
  borderCount = v.size() - borderStart;
  fineStart = v.size();
  for (float t = 10.f; t < P - 0.001f; t += 10.f)
  {
    v.append({t, 0, 0});
    v.append({t, 0, P});
    v.append({0, 0, t});
    v.append({P, 0, t});
  }
  fineCount = v.size() - fineStart;
  coarseStart = v.size();
  for (float t = 50.f; t < P - 0.001f; t += 50.f)
  {
    v.append({t, 0, 0});
    v.append({t, 0, P});
    v.append({0, 0, t});
    v.append({P, 0, t});
  }
  coarseCount = v.size() - coarseStart;
  return v;
}

// ---------------------------------------------------------------------------
// Ray-AABB (slab method)
// ---------------------------------------------------------------------------
static bool rayAABB(const QVector3D &o, const QVector3D &d,
                    const float bmin[], const float bmax[], float &t)
{
  float tmin = -FLT_MAX, tmax = FLT_MAX;
  const float ov[3] = {o.x(), o.y(), o.z()};
  const float dv[3] = {d.x(), d.y(), d.z()};
  for (int i = 0; i < 3; i++)
  {
    if (std::abs(dv[i]) < 1e-8f)
    {
      if (ov[i] < bmin[i] || ov[i] > bmax[i])
        return false;
    }
    else
    {
      float inv = 1.f / dv[i];
      float t1 = (bmin[i] - ov[i]) * inv, t2 = (bmax[i] - ov[i]) * inv;
      if (t1 > t2)
        std::swap(t1, t2);
      tmin = std::max(tmin, t1);
      tmax = std::min(tmax, t2);
      if (tmin > tmax)
        return false;
    }
  }
  t = tmin > 0.f ? tmin : tmax;
  return t > 0.f;
}

// ===========================================================================
// Constructor / Destructor
// ===========================================================================
GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer()
{
  m_vao.destroy();
  m_vbo.destroy();
  if (m_f)
  {
    for (auto &b : m_meshBatches)
    {
      if (b.vao)
        m_f->glDeleteVertexArrays(1, &b.vao);
      if (b.vbo)
        m_f->glDeleteBuffers(1, &b.vbo);
    }
    if (m_moveGizmoVao)
      m_f->glDeleteVertexArrays(1, &m_moveGizmoVao);
    if (m_moveGizmoVbo)
      m_f->glDeleteBuffers(1, &m_moveGizmoVbo);
    if (m_rotateGizmoVao)
      m_f->glDeleteVertexArrays(1, &m_rotateGizmoVao);
    if (m_rotateGizmoVbo)
      m_f->glDeleteBuffers(1, &m_rotateGizmoVbo);
    if (m_scaleGizmoVao)
      m_f->glDeleteVertexArrays(1, &m_scaleGizmoVao);
    if (m_scaleGizmoVbo)
      m_f->glDeleteBuffers(1, &m_scaleGizmoVbo);
  }
}

// ===========================================================================
// createFramebufferObject
// ===========================================================================
QOpenGLFramebufferObject *GLViewportRenderer::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4);
  m_viewSize = size;
  return new QOpenGLFramebufferObject(size, fmt);
}

// ===========================================================================
// synchronize
// ===========================================================================
void GLViewportRenderer::synchronize(QQuickFramebufferObject *item)
{
  auto *vp = static_cast<GLViewport *>(item);
  m_viewSize = QSize(int(vp->width()), int(vp->height()));

  const auto events = vp->takeEvents();
  for (const auto &e : events)
  {
    switch (e.type)
    {
    case GLViewport::InputEvent::Press:
    {
      const float sx = e.x, sy = e.y;
      if (e.button == Qt::LeftButton)
      {
        // Priority 1: gizmo axis (only if object selected)
        if (m_selectedId >= 0)
        {
          int ax = 0;
          if (m_gizmoMode == GizmoMove)
            ax = pickMoveAxis(sx, sy);
          else if (m_gizmoMode == GizmoRotate)
            ax = pickRotateAxis(sx, sy);
          else if (m_gizmoMode == GizmoScale)
            ax = pickScaleAxis(sx, sy);

          if (ax > 0)
          {
            m_gizmoAxis = ax;
            m_dragObjectId = m_selectedId;
            m_dragStartTransform = m_objectTransforms[m_selectedId];
            m_objectDragActive = true;
            m_freeXZDragging = false;
            m_mouseDragging = false;
            m_lastX = sx;
            m_lastY = sy;

            if (m_gizmoMode == GizmoMove)
            {
              auto [orig, dir] = computeRay(sx, sy);
              QVector3D axDir(ax == 1 ? 1 : 0, ax == 2 ? 1 : 0, ax == 3 ? 1 : 0);
              m_gizmoAxisT = rayToAxisT(orig, dir, axDir);
            }
            else if (m_gizmoMode == GizmoRotate)
            {
              // Record initial angle for rotation drag
              m_rotateStartAngle = computeRotateAngle(sx, sy, ax);
            }
            // Scale uses same rayToAxisT as move
            else if (m_gizmoMode == GizmoScale && ax <= 3)
            {
              auto [orig, dir] = computeRay(sx, sy);
              QVector3D axDir(ax == 1 ? 1 : 0, ax == 2 ? 1 : 0, ax == 3 ? 1 : 0);
              m_gizmoAxisT = rayToAxisT(orig, dir, axDir);
            }
            break;
          }
        }
        // Priority 2: object pick
        int hitId = pickObject(sx, sy);
        if (hitId >= 0)
        {
          m_selectedId = hitId;
          m_freeXZDragging = true;
          m_freeXZOrigin = rayXZIntersect(sx, sy);
          m_dragObjectId = m_selectedId;
          m_dragStartTransform = m_objectTransforms[m_selectedId];
          m_objectDragActive = true;
          m_mouseDragging = false;
          m_gizmoAxis = 0;
        }
        else
        {
          m_selectedId = -1;
          m_freeXZDragging = false;
          m_gizmoAxis = 0;
          m_objectDragActive = false;
          m_dragObjectId = -1;
          m_mouseDragging = true;
          m_dragButton = Qt::LeftButton;
        }
      }
      else
      {
        m_freeXZDragging = false;
        m_gizmoAxis = 0;
        m_objectDragActive = false;
        m_dragObjectId = -1;
        m_mouseDragging = true;
        m_dragButton = e.button;
      }
      m_lastX = sx;
      m_lastY = sy;
      break;
    }
    case GLViewport::InputEvent::Move:
    {
      const float dx = e.x - m_lastX, dy = e.y - m_lastY;
      if (m_gizmoAxis > 0 && m_selectedId >= 0)
      {
        if (m_gizmoMode == GizmoMove)
        {
          // Axis-constrained drag
          QVector3D axDir(m_gizmoAxis == 1 ? 1 : 0, m_gizmoAxis == 2 ? 1 : 0, m_gizmoAxis == 3 ? 1 : 0);
          auto [orig, dir] = computeRay(e.x, e.y);
          float newT = rayToAxisT(orig, dir, axDir);
          float delta = newT - m_gizmoAxisT;
          m_objectTransforms[m_selectedId].translation += delta * axDir;
          m_gizmoAxisT = newT;
        }
        else if (m_gizmoMode == GizmoRotate)
        {
          // Rotation drag
          float currentAngle = computeRotateAngle(e.x, e.y, m_gizmoAxis);
          float deltaAngle = currentAngle - m_rotateStartAngle;
          m_rotateStartAngle = currentAngle;
          QVector3D &rot = m_objectTransforms[m_selectedId].rotation;
          if (m_gizmoAxis == 1) rot.setX(rot.x() + deltaAngle);
          else if (m_gizmoAxis == 2) rot.setY(rot.y() + deltaAngle);
          else if (m_gizmoAxis == 3) rot.setZ(rot.z() + deltaAngle);
        }
        else if (m_gizmoMode == GizmoScale)
        {
          if (m_gizmoAxis <= 3)
          {
            // Axis-constrained scale
            QVector3D axDir(m_gizmoAxis == 1 ? 1 : 0, m_gizmoAxis == 2 ? 1 : 0, m_gizmoAxis == 3 ? 1 : 0);
            auto [orig, dir] = computeRay(e.x, e.y);
            float newT = rayToAxisT(orig, dir, axDir);
            float delta = newT - m_gizmoAxisT;
            // Scale by 1% per world unit of drag
            float scaleFactor = 1.0f + delta * 0.01f;
            if (scaleFactor < 0.01f) scaleFactor = 0.01f;
            QVector3D &s = m_objectTransforms[m_selectedId].scale;
            if (m_gizmoAxis == 1) s.setX(s.x() * scaleFactor);
            else if (m_gizmoAxis == 2) s.setY(s.y() * scaleFactor);
            else if (m_gizmoAxis == 3) s.setZ(s.z() * scaleFactor);
            m_gizmoAxisT = newT;
          }
          else if (m_gizmoAxis == 4)
          {
            // Uniform scale
            float screenDelta = (e.x - m_lastX) + (e.y - m_lastY);
            float scaleFactor = 1.0f + screenDelta * 0.005f;
            if (scaleFactor < 0.01f) scaleFactor = 0.01f;
            QVector3D &s = m_objectTransforms[m_selectedId].scale;
            s *= scaleFactor;
          }
        }
      }
      else if (m_freeXZDragging && m_selectedId >= 0)
      {
        // Screen-space drag: stable at any camera elevation angle.
        // Decompose screen delta into camera right / forward projected onto XZ plane.
        QMatrix4x4 view = m_camera.viewMatrix();
        // Row 0 of view matrix = camera right; Row 2 = camera back (-forward)
        QVector3D camRight(view(0, 0), 0.f, view(0, 2));
        QVector3D camFwd(-view(2, 0), 0.f, -view(2, 2)); // pointing into scene
        if (camRight.lengthSquared() < 1e-6f)
          camRight = QVector3D(1, 0, 0);
        else
          camRight.normalize();
        if (camFwd.lengthSquared() < 1e-6f)
          camFwd = QVector3D(0, 0, 1);
        else
          camFwd.normalize();
        // world units per pixel: tan(fov/2)/halfHeight * distance
        const float vH = float(m_viewSize.height());
        const float scale = m_camera.distance() * 0.4142f / (vH > 1.f ? vH * 0.5f : 1.f);
        // dx>0 → right, dy>0 (screen down) → toward viewer = -camFwd
        m_objectTransforms[m_selectedId].translation += camRight * (dx * scale) - camFwd * (dy * scale);
      }
      else if (m_mouseDragging)
      {
        if (m_dragButton == Qt::LeftButton)
          m_camera.orbit(dx * 0.5f, -dy * 0.5f);
        else if (m_dragButton == Qt::MiddleButton)
          m_camera.pan(dx, dy);
      }
      m_lastX = e.x;
      m_lastY = e.y;
      break;
    }
    case GLViewport::InputEvent::Release:
    {
      if (m_objectDragActive && m_dragObjectId >= 0)
      {
        ObjectTransform after = m_objectTransforms[m_dragObjectId];
        // Check if transform actually changed
        const ObjectTransform &before = m_dragStartTransform;
        float diff = (after.translation - before.translation).lengthSquared()
                   + (after.rotation - before.rotation).lengthSquared()
                   + (after.scale - before.scale).lengthSquared();
        if (diff > 1e-8f)
        {
          TransformCmd cmd;
          cmd.objectId = m_dragObjectId;
          cmd.before = before;
          cmd.after = after;
          m_undoStack.push_back(cmd);
          if (m_undoStack.size() > 100)
            m_undoStack.erase(m_undoStack.begin());
          m_redoStack.clear();
        }
      }
      m_mouseDragging = false;
      m_freeXZDragging = false;
      m_gizmoAxis = 0;
      m_objectDragActive = false;
      m_dragObjectId = -1;
      break;
    }
    case GLViewport::InputEvent::Wheel:
      m_camera.zoom(e.wheelDelta);
      break;
    case GLViewport::InputEvent::FitView:
      m_camera.fitView(e.fitCX, e.fitCY, e.fitCZ, e.fitRadius);
      break;
    case GLViewport::InputEvent::ViewPreset:
      switch (int(e.x))
      {
      case 0: m_camera.viewTop(); break;
      case 1: m_camera.viewFront(); break;
      case 2: m_camera.viewRight(); break;
      case 3: m_camera.viewIso(); break;
      default: m_camera.viewIso(); break;
      }
      break;
    case GLViewport::InputEvent::Undo:
      if (!m_undoStack.empty())
      {
        TransformCmd cmd = m_undoStack.back();
        m_undoStack.pop_back();
        m_objectTransforms[cmd.objectId] = cmd.before;
        m_redoStack.push_back(cmd);
        m_selectedId = cmd.objectId;
      }
      break;
    case GLViewport::InputEvent::Redo:
      if (!m_redoStack.empty())
      {
        TransformCmd cmd = m_redoStack.back();
        m_redoStack.pop_back();
        m_objectTransforms[cmd.objectId] = cmd.after;
        m_undoStack.push_back(cmd);
        m_selectedId = cmd.objectId;
      }
      break;
    case GLViewport::InputEvent::ClearHistory:
      m_undoStack.clear();
      m_redoStack.clear();
      break;
    case GLViewport::InputEvent::SetGizmoMode:
      m_gizmoMode = static_cast<GizmoMode>(int(e.x));
      m_gizmoAxis = 0;
      break;
    }
  }

  QByteArray newMesh;
  if (vp->takeMesh(newMesh, m_meshVersion))
  {
    m_pendingMesh = std::move(newMesh);
    m_meshDirty = true;
  }

  m_wireframeMode = vp->wireframeMode();
}

// ===========================================================================
// render
// ===========================================================================
void GLViewportRenderer::render()
{
  if (!m_initialized)
    initialize();
  if (m_meshDirty)
  {
    uploadMesh();
    m_meshDirty = false;
  }

  // Update gizmo center to follow selected object
  if (m_selectedId >= 0)
  {
    for (const auto &b : m_meshBatches)
    {
      if (b.objectId == m_selectedId)
      {
        const ObjectTransform &t = m_objectTransforms[m_selectedId];
        m_gizmoCenter = QVector3D(
            (b.bboxMin[0] + b.bboxMax[0]) * 0.5f + t.translation.x(),
            (b.bboxMin[1] + b.bboxMax[1]) * 0.5f + t.translation.y(),
            (b.bboxMin[2] + b.bboxMax[2]) * 0.5f + t.translation.z());
        break;
      }
    }
  }

  m_f->glEnable(GL_DEPTH_TEST);
  m_f->glDisable(GL_CULL_FACE);
  m_f->glClearColor(0.208f, 0.224f, 0.243f, 1.f);
  m_f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = m_viewSize.height() > 0
                           ? float(m_viewSize.width()) / float(m_viewSize.height())
                           : 1.f;
  const QMatrix4x4 mvp = m_camera.projMatrix(aspect) * m_camera.viewMatrix();

  // 1. Mesh batches
  if (!m_meshBatches.empty())
  {
    m_meshProg.bind();
    m_meshProg.setUniformValue(m_uMeshMVP, mvp);

    // Wireframe mode: switch to GL_LINE rendering (matching upstream Plater::toggle_show_wireframe)
    if (m_wireframeMode && m_glPolygonMode)
      m_glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (auto &b : m_meshBatches)
    {
      if (b.vertexCount <= 0)
        continue;
      QMatrix4x4 model = computeModelMatrix(b.objectId);
      m_meshProg.setUniformValue(m_uMeshModel, model);
      if (m_wireframeMode)
      {
        // Upstream uses DARK_GRAY for wireframe rendering
        m_meshProg.setUniformValue(m_uMeshBaseColor, QVector3D(0.35f, 0.35f, 0.35f));
        m_meshProg.setUniformValue(m_uMeshBright, 1.0f);
        m_f->glLineWidth(1.0f);
      }
      else
      {
        m_meshProg.setUniformValue(m_uMeshBaseColor, kObjColors[b.objectId % kNumColors]);
        float bright = (b.objectId == m_selectedId) ? 1.55f : 1.0f;
        m_meshProg.setUniformValue(m_uMeshBright, bright);
      }
      m_f->glBindVertexArray(b.vao);
      m_f->glDrawArrays(GL_TRIANGLES, 0, b.vertexCount);
      m_f->glBindVertexArray(0);
    }

    if (m_wireframeMode && m_glPolygonMode)
      m_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    m_meshProg.release();
  }

  // 2. Gizmo (no depth test fighting with mesh)
  if (m_selectedId >= 0)
    renderGizmo(mvp);

  // 3. Grid / axes
  m_prog.bind();
  m_prog.setUniformValue(m_uMVP, mvp);
  m_vao.bind();
  m_prog.setUniformValue(m_uColor, QVector4D(0.28f, 0.30f, 0.32f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_fineGridStart, m_fineGridCount);
  m_prog.setUniformValue(m_uColor, QVector4D(0.42f, 0.44f, 0.47f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_coarseGridStart, m_coarseGridCount);
  m_prog.setUniformValue(m_uColor, QVector4D(0.85f, 0.85f, 0.85f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_borderStart, m_borderCount);
  m_prog.setUniformValue(m_uColor, QVector4D(0.85f, 0.18f, 0.18f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 0, 2);
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.78f, 0.22f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 2, 2);
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.38f, 0.88f, 1.f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 4, 2);
  m_vao.release();
  m_prog.release();

  m_f->glDisable(GL_DEPTH_TEST);
}

// ===========================================================================
// renderGizmo
// ===========================================================================
void GLViewportRenderer::renderGizmo(const QMatrix4x4 &mvp)
{
  if (m_gizmoMode == GizmoMove)
    renderMoveGizmo(mvp);
  else if (m_gizmoMode == GizmoRotate)
    renderRotateGizmo(mvp);
  else if (m_gizmoMode == GizmoScale)
    renderScaleGizmo(mvp);
}

// ===========================================================================
// renderMoveGizmo
// ===========================================================================
void GLViewportRenderer::renderMoveGizmo(const QMatrix4x4 &mvp)
{
  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = dist * 0.15f;
  if (scale < 5.f)
    scale = 5.f;

  m_f->glClear(GL_DEPTH_BUFFER_BIT);
  m_f->glDisable(GL_CULL_FACE);

  m_gizmoProg.bind();
  m_gizmoProg.setUniformValue(m_uGizmoMVP, mvp);
  m_gizmoProg.setUniformValue(m_uGizmoCenter, m_gizmoCenter);
  m_gizmoProg.setUniformValue(m_uGizmoScale, scale);

  m_f->glBindVertexArray(m_moveGizmoVao);

  static constexpr int CONE_VERTS_STATIC = 6 * 3 * 2;

  for (int ax = 0; ax < 3; ax++)
  {
    bool active = (m_gizmoAxis == ax + 1);
    QVector4D col = active ? kAxisHighlight[ax] : kAxisColors[ax];
    m_gizmoProg.setUniformValue(m_uGizmoColor, col);
    m_f->glDrawArrays(GL_LINES, m_moveShaftBase[ax], 2);
    m_f->glDrawArrays(GL_TRIANGLES, m_moveConeBase[ax], CONE_VERTS_STATIC);
  }

  m_f->glBindVertexArray(0);
  m_gizmoProg.release();
  m_f->glEnable(GL_CULL_FACE);
}

// ===========================================================================
// initialize
// ===========================================================================
void GLViewportRenderer::initialize()
{
  m_f = QOpenGLContext::currentContext()->extraFunctions();
  m_f->initializeOpenGLFunctions();

  // Resolve glPolygonMode (not in QOpenGLExtraFunctions on Qt 6.10)
  m_glPolygonMode = reinterpret_cast<GlPolygonModeFn>(
      QOpenGLContext::currentContext()->getProcAddress("glPolygonMode"));

  m_prog.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertSrc);
  m_prog.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
  if (!m_prog.link())
    qWarning("[GL] base: %s", qPrintable(m_prog.log()));
  m_uMVP = m_prog.uniformLocation("uMVP");
  m_uColor = m_prog.uniformLocation("uColor");

  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Vertex, kMeshVertSrc);
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFragSrc);
  if (!m_meshProg.link())
    qWarning("[GL] mesh: %s", qPrintable(m_meshProg.log()));
  m_uMeshMVP = m_meshProg.uniformLocation("uMVP");
  m_uMeshModel = m_meshProg.uniformLocation("uModel");
  m_uMeshBaseColor = m_meshProg.uniformLocation("uBaseColor");
  m_uMeshBright = m_meshProg.uniformLocation("uBrightness");

  m_gizmoProg.addShaderFromSourceCode(QOpenGLShader::Vertex, kGizmoVertSrc);
  m_gizmoProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kGizmoFragSrc);
  if (!m_gizmoProg.link())
    qWarning("[GL] gizmo: %s", qPrintable(m_gizmoProg.log()));
  m_uGizmoMVP = m_gizmoProg.uniformLocation("uMVP");
  m_uGizmoCenter = m_gizmoProg.uniformLocation("uGizmoCenter");
  m_uGizmoScale = m_gizmoProg.uniformLocation("uGizmoScale");
  m_uGizmoColor = m_gizmoProg.uniformLocation("uGizmoColor");

  const auto verts = buildGeometry(
      m_axisStart, m_axisCount, m_borderStart, m_borderCount,
      m_fineGridStart, m_fineGridCount, m_coarseGridStart, m_coarseGridCount);
  m_vao.create();
  m_vao.bind();
  m_vbo.create();
  m_vbo.bind();
  m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vbo.allocate(verts.constData(), int(verts.size() * sizeof(GVertex)));
  m_prog.enableAttributeArray(0);
  m_prog.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GVertex));
  m_vao.release();
  m_vbo.release();

  buildGizmoGeometry();
  buildRotateGizmoGeometry();
  buildScaleGizmoGeometry();
  m_initialized = true;
}

// ===========================================================================
// buildGizmoGeometry  -- builds static XYZ arrow geometry in local space
// ===========================================================================
void GLViewportRenderer::buildGizmoGeometry()
{
  static const float kGizmoVerts[][3] = {
      {0.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, 0.05500f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, 0.02750f, 0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, -0.05500f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, -0.02750f, -0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.00000f, 0.00000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {-0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.05500f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.05500f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {-0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 0.00000f, 0.00000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.02750f, 0.04763f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.02750f, -0.04763f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
  };
  static constexpr int kGizmoN = 114;
  m_moveShaftBase[0] = 0;
  m_moveShaftBase[1] = 38;
  m_moveShaftBase[2] = 76;
  m_moveConeBase[0] = 2;
  m_moveConeBase[1] = 40;
  m_moveConeBase[2] = 78;

  m_f->glGenVertexArrays(1, &m_moveGizmoVao);
  m_f->glGenBuffers(1, &m_moveGizmoVbo);
  m_f->glBindVertexArray(m_moveGizmoVao);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, m_moveGizmoVbo);
  m_f->glBufferData(GL_ARRAY_BUFFER, sizeof(kGizmoVerts), kGizmoVerts, GL_STATIC_DRAW);
  m_f->glEnableVertexAttribArray(0);
  m_f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  m_f->glBindVertexArray(0);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ===========================================================================
// uploadMesh
// ===========================================================================
void GLViewportRenderer::uploadMesh()
{
  try
  {
    for (auto &b : m_meshBatches)
    {
      if (b.vao)
        m_f->glDeleteVertexArrays(1, &b.vao);
      if (b.vbo)
        m_f->glDeleteBuffers(1, &b.vbo);
    }
    m_meshBatches.clear();

    if (m_pendingMesh.size() < int(sizeof(int32_t)))
      return;
    const char *p = m_pendingMesh.constData(), *end = p + m_pendingMesh.size();
    int32_t objCount = 0;
    memcpy(&objCount, p, sizeof(int32_t));
    p += sizeof(int32_t);
    if (objCount <= 0)
    {
      qWarning("[GL] uploadMesh ignored: invalid object count %d", int(objCount));
      return;
    }
    const ptrdiff_t remainBytes = end - p;
    const ptrdiff_t minPerObject = 2 * ptrdiff_t(sizeof(int32_t));
    if (remainBytes < 0 || minPerObject <= 0 || objCount > remainBytes / minPerObject)
    {
      qWarning("[GL] uploadMesh ignored: malformed header objCount=%d remain=%lld", int(objCount), static_cast<long long>(remainBytes));
      return;
    }
    m_meshBatches.reserve(objCount);

    for (int32_t i = 0; i < objCount; ++i)
    {
      if (p + 2 * int(sizeof(int32_t)) > end)
        break;
      int32_t objId = 0, triCount = 0;
      memcpy(&objId, p, sizeof(int32_t));
      p += sizeof(int32_t);
      memcpy(&triCount, p, sizeof(int32_t));
      p += sizeof(int32_t);
      if (triCount <= 0)
      {
        qWarning("[GL] uploadMesh skip object %d: triCount=%d", int(objId), int(triCount));
        continue;
      }
      const ptrdiff_t dataBytes64 = ptrdiff_t(triCount) * 9 * ptrdiff_t(sizeof(float));
      if (dataBytes64 <= 0)
      {
        qWarning("[GL] uploadMesh skip object %d: invalid data size", int(objId));
        continue;
      }
      if (dataBytes64 > (end - p))
      {
        qWarning("[GL] uploadMesh stop: object %d exceeds payload", int(objId));
        break;
      }
      if (dataBytes64 > ptrdiff_t(std::numeric_limits<int>::max()))
      {
        qWarning("[GL] uploadMesh stop: object %d data too large", int(objId));
        break;
      }
      const int dataBytes = int(dataBytes64);
      if (p + dataBytes > end)
        break;

      MeshBatch b;
      b.objectId = int(objId);
      b.vertexCount = triCount * 3;
      const float *fp = reinterpret_cast<const float *>(p);
      b.bboxMin[0] = b.bboxMax[0] = fp[0];
      b.bboxMin[1] = b.bboxMax[1] = fp[1];
      b.bboxMin[2] = b.bboxMax[2] = fp[2];
      for (int vi = 0; vi < triCount * 3; vi++)
        for (int ci = 0; ci < 3; ci++)
        {
          float v = fp[vi * 3 + ci];
          if (v < b.bboxMin[ci])
            b.bboxMin[ci] = v;
          if (v > b.bboxMax[ci])
            b.bboxMax[ci] = v;
        }

      m_f->glGenVertexArrays(1, &b.vao);
      m_f->glGenBuffers(1, &b.vbo);
      m_f->glBindVertexArray(b.vao);
      m_f->glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
      m_f->glBufferData(GL_ARRAY_BUFFER, dataBytes, p, GL_DYNAMIC_DRAW);
      m_f->glEnableVertexAttribArray(0);
      m_f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
      m_f->glBindVertexArray(0);
      m_f->glBindBuffer(GL_ARRAY_BUFFER, 0);
      m_meshBatches.push_back(b);
      p += dataBytes;
    }

    std::unordered_set<int> aliveIds;
    aliveIds.reserve(m_meshBatches.size() * 2 + 1);
    for (const auto &b : m_meshBatches)
      aliveIds.insert(b.objectId);

    for (auto it = m_objectTransforms.begin(); it != m_objectTransforms.end();)
    {
      if (aliveIds.find(it->first) == aliveIds.end())
        it = m_objectTransforms.erase(it);
      else
        ++it;
    }

    m_undoStack.erase(
        std::remove_if(m_undoStack.begin(), m_undoStack.end(), [&aliveIds](const TransformCmd &cmd)
                       { return aliveIds.find(cmd.objectId) == aliveIds.end(); }),
        m_undoStack.end());
    m_redoStack.erase(
        std::remove_if(m_redoStack.begin(), m_redoStack.end(), [&aliveIds](const TransformCmd &cmd)
                       { return aliveIds.find(cmd.objectId) == aliveIds.end(); }),
        m_redoStack.end());

    if (m_selectedId >= 0 && aliveIds.find(m_selectedId) == aliveIds.end())
      m_selectedId = -1;

    if (!m_meshBatches.empty())
    {
      float gmin[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
      float gmax[3] = {-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

      for (const auto &b : m_meshBatches)
      {
        for (int i = 0; i < 3; ++i)
        {
          gmin[i] = std::min(gmin[i], b.bboxMin[i]);
          gmax[i] = std::max(gmax[i], b.bboxMax[i]);
        }
      }

      const float cx = (gmin[0] + gmax[0]) * 0.5f;
      const float cy = (gmin[1] + gmax[1]) * 0.5f;
      const float cz = (gmin[2] + gmax[2]) * 0.5f;
      const float dx = gmax[0] - gmin[0];
      const float dy = gmax[1] - gmin[1];
      const float dz = gmax[2] - gmin[2];
      const float radius = std::max(10.0f, std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f);
      m_camera.fitView(cx, cy, cz, radius);
    }

    int tt = 0;
    for (const auto &b : m_meshBatches)
      tt += b.vertexCount / 3;
    qInfo("[GL] %d batches, %d tris", int(m_meshBatches.size()), tt);
  }
  catch (const std::exception &ex)
  {
    qWarning("[GL] uploadMesh exception: %s", ex.what());
    m_meshBatches.clear();
  }
  catch (...)
  {
    qWarning("[GL] uploadMesh exception: unknown");
    m_meshBatches.clear();
  }
}

// ===========================================================================
// computeRay
// ===========================================================================
std::pair<QVector3D, QVector3D>
GLViewportRenderer::computeRay(float sx, float sy) const
{
  const float w = float(m_viewSize.width()), h = float(m_viewSize.height());
  if (w <= 0 || h <= 0)
    return {{}, {1, 0, 0}};
  QMatrix4x4 invPV = (m_camera.projMatrix(w / h) * m_camera.viewMatrix()).inverted();
  float ndcX = 2.f * sx / w - 1.f, ndcY = 1.f - 2.f * sy / h;
  QVector4D nearH = invPV * QVector4D(ndcX, ndcY, -1.f, 1.f);
  QVector4D farH = invPV * QVector4D(ndcX, ndcY, 1.f, 1.f);
  nearH /= nearH.w();
  farH /= farH.w();
  QVector3D orig = nearH.toVector3D();
  return {orig, (farH.toVector3D() - orig).normalized()};
}

// ===========================================================================
// rayXZIntersect
// ===========================================================================
QVector3D GLViewportRenderer::rayXZIntersect(float sx, float sy) const
{
  auto [orig, dir] = computeRay(sx, sy);
  if (std::abs(dir.y()) < 1e-6f)
    return {0, 0, 0};
  float t = -orig.y() / dir.y();
  return orig + t * dir;
}

// ===========================================================================
// pickObject
// ===========================================================================
int GLViewportRenderer::pickObject(float sx, float sy) const
{
  auto [orig, dir] = computeRay(sx, sy);
  float tBest = FLT_MAX;
  int hitId = -1;
  for (const auto &b : m_meshBatches)
  {
    float bmin[3], bmax[3];
    transformedAABB(b.objectId, bmin, bmax);
    float t;
    if (rayAABB(orig, dir, bmin, bmax, t) && t < tBest)
    {
      tBest = t;
      hitId = b.objectId;
    }
  }
  return hitId;
}

// ===========================================================================
// rayToAxisT  -- closest t along axisDir at gizmoCenter from screen ray
// ===========================================================================
float GLViewportRenderer::rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                                     const QVector3D &axisDir) const
{
  // Solve shortest distance between two lines:
  // L1: orig + t*dir   L2: m_gizmoCenter + s*axisDir
  QVector3D w = orig - m_gizmoCenter;
  float b = QVector3D::dotProduct(dir, axisDir);
  float e = QVector3D::dotProduct(axisDir, w);
  float d = QVector3D::dotProduct(dir, w);
  float denom = 1.f - b * b;
  if (std::abs(denom) < 1e-8f)
    return e; // parallel
  return (e - b * d) / denom;
}

// ===========================================================================
// pickMoveAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
// ===========================================================================
int GLViewportRenderer::pickMoveAxis(float sx, float sy) const
{
  if (m_meshBatches.empty())
    return 0;

  // Compute gizmo scale for proximity threshold
  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = std::max(dist * 0.15f, 5.f);
  // Threshold = 8% of arrow length in world units
  float thresh = scale * 0.08f;

  auto [orig, dir] = computeRay(sx, sy);

  static const QVector3D kAxes[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;
  for (int ax = 0; ax < 3; ax++)
  {
    // Closest distance from ray to axis line segment
    QVector3D axDir = kAxes[ax];
    QVector3D w = orig - m_gizmoCenter;
    float b = QVector3D::dotProduct(dir, axDir);
    float e = QVector3D::dotProduct(axDir, w);
    float d = QVector3D::dotProduct(dir, w);
    float denom = 1.f - b * b;
    float t_ray, s_axis;
    if (std::abs(denom) < 1e-8f)
    {
      t_ray = 0;
      s_axis = e;
    }
    else
    {
      t_ray = (b * e - d) / denom;
      s_axis = (e - b * d) / denom;
    }
    if (t_ray < 0)
      continue;      // behind camera
    s_axis /= scale; // normalise to [0..1] range
    if (s_axis < 0 || s_axis > 1.0f)
      continue; // outside arrow length

    QVector3D p1 = orig + t_ray * dir;
    QVector3D p2 = m_gizmoCenter + s_axis * scale * axDir;
    float dist2 = (p1 - p2).length();
    if (dist2 < thresh && dist2 < bestDist)
    {
      bestDist = dist2;
      best = ax + 1;
    }
  }
  return best;
}

// ===========================================================================
// computeModelMatrix  -- build T * R * S model matrix for an object
// ===========================================================================
QMatrix4x4 GLViewportRenderer::computeModelMatrix(int objectId) const
{
  auto it = m_objectTransforms.find(objectId);
  const ObjectTransform t = (it != m_objectTransforms.end()) ? it->second : ObjectTransform{};
  QMatrix4x4 model;
  model.translate(t.translation);
  model.rotate(t.rotation.x(), 1, 0, 0);
  model.rotate(t.rotation.y(), 0, 1, 0);
  model.rotate(t.rotation.z(), 0, 0, 1);
  model.scale(t.scale);
  return model;
}

// ===========================================================================
// transformedAABB  -- compute world-space AABB for picking
// ===========================================================================
void GLViewportRenderer::transformedAABB(int objectId, float outMin[3], float outMax[3]) const
{
  // Find original bbox
  const float *bmin = nullptr, *bmax = nullptr;
  for (const auto &b : m_meshBatches)
  {
    if (b.objectId == objectId)
    {
      bmin = b.bboxMin;
      bmax = b.bboxMax;
      break;
    }
  }
  if (!bmin)
  {
    for (int i = 0; i < 3; i++) { outMin[i] = -FLT_MAX; outMax[i] = FLT_MAX; }
    return;
  }

  QMatrix4x4 model = computeModelMatrix(objectId);

  // Transform all 8 corners and find new AABB
  float corners[8][3];
  int ci = 0;
  for (int x = 0; x < 2; x++)
    for (int y = 0; y < 2; y++)
      for (int z = 0; z < 2; z++)
      {
        QVector4D wp = model * QVector4D(
            x ? bmax[0] : bmin[0],
            y ? bmax[1] : bmin[1],
            z ? bmax[2] : bmin[2], 1.f);
        corners[ci][0] = wp.x();
        corners[ci][1] = wp.y();
        corners[ci][2] = wp.z();
        ci++;
      }

  for (int i = 0; i < 3; i++)
  {
    outMin[i] = outMax[i] = corners[0][i];
    for (int j = 1; j < 8; j++)
    {
      if (corners[j][i] < outMin[i]) outMin[i] = corners[j][i];
      if (corners[j][i] > outMax[i]) outMax[i] = corners[j][i];
    }
  }
}

// ===========================================================================
// computeRotateAngle  -- compute angle for rotate gizmo interaction
// ===========================================================================
float GLViewportRenderer::computeRotateAngle(float sx, float sy, int axis) const
{
  // Project mouse position onto the rotation plane and compute angle
  QVector3D planeNormal;
  if (axis == 1) planeNormal = QVector3D(1, 0, 0);
  else if (axis == 2) planeNormal = QVector3D(0, 1, 0);
  else planeNormal = QVector3D(0, 0, 1);

  auto [orig, dir] = computeRay(sx, sy);
  float denom = QVector3D::dotProduct(dir, planeNormal);
  if (std::abs(denom) < 1e-6f)
    return m_rotateStartAngle; // ray parallel to plane

  float t = QVector3D::dotProduct(m_gizmoCenter - orig, planeNormal) / denom;
  QVector3D hit = orig + t * dir;
  QVector3D v = hit - m_gizmoCenter;

  // Compute angle on the rotation plane
  // Reference axis: the "right" direction on this plane
  QVector3D ref;
  if (axis == 1) ref = QVector3D(0, 1, 0); // Y-axis as reference for X rotation
  else if (axis == 2) ref = QVector3D(1, 0, 0); // X-axis as reference for Y rotation
  else ref = QVector3D(1, 0, 0); // X-axis as reference for Z rotation

  return std::atan2(QVector3D::dotProduct(QVector3D::crossProduct(ref, v).normalized(), planeNormal),
                    QVector3D::dotProduct(ref, v));
}

// ===========================================================================
// buildRotateGizmoGeometry  -- build 3 torus rings for rotation gizmo
// ===========================================================================
void GLViewportRenderer::buildRotateGizmoGeometry()
{
  const int segments = 48;
  const int sides = 6;
  const float majorR = 0.7f; // ring center radius
  const float minorR = 0.035f; // tube radius

  QVector<float> verts;
  verts.reserve(3 * segments * sides * 6); // 6 verts per quad

  // Generate a torus ring in a specific plane
  // axisIndex: 0=X (YZ plane), 1=Y (XZ plane), 2=Z (XY plane)
  auto addRing = [&](int axisIndex)
  {
    int baseVertex = verts.size() / 3;
    for (int i = 0; i < segments; i++)
    {
      float theta1 = 2.f * M_PI * i / segments;
      float theta2 = 2.f * M_PI * (i + 1) / segments;

      for (int j = 0; j < sides; j++)
      {
        float phi1 = 2.f * M_PI * j / sides;
        float phi2 = 2.f * M_PI * (j + 1) / sides;

        auto point = [&](float theta, float phi) -> QVector3D
        {
          float r = majorR + minorR * std::cos(phi);
          float y = minorR * std::sin(phi);
          // Base ring in XZ plane
          float x = r * std::cos(theta);
          float z = r * std::sin(theta);
          // Rotate to target plane
          if (axisIndex == 0) return QVector3D(y, x, z);     // X ring: YZ plane
          else if (axisIndex == 1) return QVector3D(x, y, z); // Y ring: XZ plane
          else return QVector3D(x, z, y);                     // Z ring: XY plane
        };

        QVector3D p00 = point(theta1, phi1);
        QVector3D p10 = point(theta2, phi1);
        QVector3D p01 = point(theta1, phi2);
        QVector3D p11 = point(theta2, phi2);

        // Triangle 1
        verts.append(p00.x()); verts.append(p00.y()); verts.append(p00.z());
        verts.append(p10.x()); verts.append(p10.y()); verts.append(p10.z());
        verts.append(p11.x()); verts.append(p11.y()); verts.append(p11.z());
        // Triangle 2
        verts.append(p00.x()); verts.append(p00.y()); verts.append(p00.z());
        verts.append(p11.x()); verts.append(p11.y()); verts.append(p11.z());
        verts.append(p01.x()); verts.append(p01.y()); verts.append(p01.z());
      }
    }
    return baseVertex;
  };

  m_rotateRingBase[0] = addRing(0); // X ring
  m_rotateRingBase[1] = addRing(1); // Y ring
  m_rotateRingBase[2] = addRing(2); // Z ring
  m_rotateRingVertCount = segments * sides * 6; // vertices per ring

  m_f->glGenVertexArrays(1, &m_rotateGizmoVao);
  m_f->glGenBuffers(1, &m_rotateGizmoVbo);
  m_f->glBindVertexArray(m_rotateGizmoVao);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, m_rotateGizmoVbo);
  m_f->glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.constData(), GL_STATIC_DRAW);
  m_f->glEnableVertexAttribArray(0);
  m_f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  m_f->glBindVertexArray(0);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ===========================================================================
// renderRotateGizmo
// ===========================================================================
void GLViewportRenderer::renderRotateGizmo(const QMatrix4x4 &mvp)
{
  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = dist * 0.15f;
  if (scale < 5.f)
    scale = 5.f;

  m_f->glClear(GL_DEPTH_BUFFER_BIT);
  m_f->glDisable(GL_CULL_FACE);

  m_gizmoProg.bind();
  m_gizmoProg.setUniformValue(m_uGizmoMVP, mvp);
  m_gizmoProg.setUniformValue(m_uGizmoCenter, m_gizmoCenter);
  m_gizmoProg.setUniformValue(m_uGizmoScale, scale);

  m_f->glBindVertexArray(m_rotateGizmoVao);

  for (int ax = 0; ax < 3; ax++)
  {
    bool active = (m_gizmoAxis == ax + 1);
    QVector4D col = active ? kAxisHighlight[ax] : kAxisColors[ax];
    m_gizmoProg.setUniformValue(m_uGizmoColor, col);
    m_f->glDrawArrays(GL_TRIANGLES, m_rotateRingBase[ax], m_rotateRingVertCount);
  }

  m_f->glBindVertexArray(0);
  m_gizmoProg.release();
  m_f->glEnable(GL_CULL_FACE);
}

// ===========================================================================
// pickRotateAxis  -- returns 1/2/3 for X/Y/Z ring, 0 if no hit
// ===========================================================================
int GLViewportRenderer::pickRotateAxis(float sx, float sy) const
{
  if (m_meshBatches.empty())
    return 0;

  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = std::max(dist * 0.15f, 5.f);
  float thresh = scale * 0.10f; // picking tolerance

  auto [orig, dir] = computeRay(sx, sy);

  // For each ring, project ray onto ring plane and check distance to ring
  static const QVector3D kNormals[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;

  for (int ax = 0; ax < 3; ax++)
  {
    const QVector3D &n = kNormals[ax];
    float denom = QVector3D::dotProduct(dir, n);
    if (std::abs(denom) < 1e-6f)
      continue; // ray parallel to plane

    float t = QVector3D::dotProduct(m_gizmoCenter - orig, n) / denom;
    if (t < 0)
      continue; // behind camera

    QVector3D hit = orig + t * dir;
    float ringDist = (hit - m_gizmoCenter).length();
    float tubeDist = std::abs(ringDist - scale * 0.7f); // 0.7 = majorR

    if (tubeDist < thresh && tubeDist < bestDist)
    {
      bestDist = tubeDist;
      best = ax + 1;
    }
  }
  return best;
}

// ===========================================================================
// buildScaleGizmoGeometry  -- build 3 axis shafts + box handles
// ===========================================================================
void GLViewportRenderer::buildScaleGizmoGeometry()
{
  QVector<float> verts;
  const float shaftEnd = 0.7f;
  const float boxSize = 0.08f;
  const float boxCenter = 0.78f;

  // Generate a unit cube centered at origin
  auto addCube = [&](const QVector3D &center, const QVector3D &size)
  {
    float hx = size.x() * 0.5f, hy = size.y() * 0.5f, hz = size.z() * 0.5f;
    float cx = center.x(), cy = center.y(), cz = center.z();

    // 8 corners
    QVector3D c[8] = {
        {cx - hx, cy - hy, cz - hz}, {cx + hx, cy - hy, cz - hz},
        {cx + hx, cy + hy, cz - hz}, {cx - hx, cy + hy, cz - hz},
        {cx - hx, cy - hy, cz + hz}, {cx + hx, cy - hy, cz + hz},
        {cx + hx, cy + hy, cz + hz}, {cx - hx, cy + hy, cz + hz}};

    // 12 triangles (6 faces)
    int faces[6][4] = {
        {0, 1, 2, 3}, // -Z
        {4, 6, 5, 7}, // +Z
        {0, 4, 5, 1}, // -Y
        {2, 6, 7, 3}, // +Y
        {0, 3, 7, 4}, // -X
        {1, 5, 6, 2}, // +X
    };

    for (auto &f : faces)
    {
      auto &a = c[f[0]], &b = c[f[1]], &cc = c[f[2]], &d = c[f[3]];
      // Tri 1: a b c
      verts.append(a.x()); verts.append(a.y()); verts.append(a.z());
      verts.append(b.x()); verts.append(b.y()); verts.append(b.z());
      verts.append(cc.x()); verts.append(cc.y()); verts.append(cc.z());
      // Tri 2: a c d
      verts.append(a.x()); verts.append(a.y()); verts.append(a.z());
      verts.append(cc.x()); verts.append(cc.y()); verts.append(cc.z());
      verts.append(d.x()); verts.append(d.y()); verts.append(d.z());
    }
  };

  static const QVector3D kAxes[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  for (int ax = 0; ax < 3; ax++)
  {
    m_scaleShaftBase[ax] = verts.size() / 3;
    // Shaft line (origin to shaftEnd)
    QVector3D o(0, 0, 0);
    QVector3D e = kAxes[ax] * shaftEnd;
    verts.append(o.x()); verts.append(o.y()); verts.append(o.z());
    verts.append(e.x()); verts.append(e.y()); verts.append(e.z());

    m_scaleBoxBase[ax] = verts.size() / 3;
    // Box handle at end of axis
    addCube(kAxes[ax] * boxCenter, QVector3D(boxSize, boxSize, boxSize));
  }
  m_scaleBoxVertCount = 36; // 6 faces * 2 tris * 3 verts

  m_f->glGenVertexArrays(1, &m_scaleGizmoVao);
  m_f->glGenBuffers(1, &m_scaleGizmoVbo);
  m_f->glBindVertexArray(m_scaleGizmoVao);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, m_scaleGizmoVbo);
  m_f->glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.constData(), GL_STATIC_DRAW);
  m_f->glEnableVertexAttribArray(0);
  m_f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  m_f->glBindVertexArray(0);
  m_f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ===========================================================================
// renderScaleGizmo
// ===========================================================================
void GLViewportRenderer::renderScaleGizmo(const QMatrix4x4 &mvp)
{
  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = dist * 0.15f;
  if (scale < 5.f)
    scale = 5.f;

  m_f->glClear(GL_DEPTH_BUFFER_BIT);
  m_f->glDisable(GL_CULL_FACE);

  m_gizmoProg.bind();
  m_gizmoProg.setUniformValue(m_uGizmoMVP, mvp);
  m_gizmoProg.setUniformValue(m_uGizmoCenter, m_gizmoCenter);
  m_gizmoProg.setUniformValue(m_uGizmoScale, scale);

  m_f->glBindVertexArray(m_scaleGizmoVao);

  for (int ax = 0; ax < 3; ax++)
  {
    bool active = (m_gizmoAxis == ax + 1);
    QVector4D col = active ? kAxisHighlight[ax] : kAxisColors[ax];
    m_gizmoProg.setUniformValue(m_uGizmoColor, col);
    m_f->glDrawArrays(GL_LINES, m_scaleShaftBase[ax], 2);
    m_f->glDrawArrays(GL_TRIANGLES, m_scaleBoxBase[ax], m_scaleBoxVertCount);
  }

  m_f->glBindVertexArray(0);
  m_gizmoProg.release();
  m_f->glEnable(GL_CULL_FACE);
}

// ===========================================================================
// pickScaleAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
// ===========================================================================
int GLViewportRenderer::pickScaleAxis(float sx, float sy) const
{
  if (m_meshBatches.empty())
    return 0;

  float dist = (m_gizmoCenter - m_camera.eye()).length();
  float scale = std::max(dist * 0.15f, 5.f);
  float thresh = scale * 0.10f;

  auto [orig, dir] = computeRay(sx, sy);

  static const QVector3D kAxes[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;
  for (int ax = 0; ax < 3; ax++)
  {
    QVector3D axDir = kAxes[ax];
    QVector3D w = orig - m_gizmoCenter;
    float b = QVector3D::dotProduct(dir, axDir);
    float e = QVector3D::dotProduct(axDir, w);
    float d = QVector3D::dotProduct(dir, w);
    float denom = 1.f - b * b;
    float t_ray, s_axis;
    if (std::abs(denom) < 1e-8f)
    {
      t_ray = 0;
      s_axis = e;
    }
    else
    {
      t_ray = (b * e - d) / denom;
      s_axis = (e - b * d) / denom;
    }
    if (t_ray < 0)
      continue;
    s_axis /= scale;
    if (s_axis < 0 || s_axis > 1.0f)
      continue;

    QVector3D p1 = orig + t_ray * dir;
    QVector3D p2 = m_gizmoCenter + s_axis * scale * axDir;
    float d2 = (p1 - p2).length();
    if (d2 < thresh && d2 < bestDist)
    {
      bestDist = d2;
      best = ax + 1;
    }
  }
  return best;
}
