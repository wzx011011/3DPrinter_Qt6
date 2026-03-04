#include "GLViewportRenderer.h"
#include "GLViewport.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector>
#include <QOpenGLExtraFunctions>
#include <cstring>

// --- Basic shaders (grid / axes) ---
static const char *kVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "void main() { gl_Position = uMVP * vec4(aPos, 1.0); }\n";

static const char *kFragSrc =
    "#version 330 core\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main() { fragColor = uColor; }\n";

// --- Mesh shader: flat-shaded + per-object colour ---
static const char *kMeshVertSrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "uniform mat4 uMVP;\n"
    "out vec3 vWorldPos;\n"
    "void main() {\n"
    "  vWorldPos = aPos;\n"
    "  gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "}\n";

static const char *kMeshFragSrc =
    "#version 330 core\n"
    "in  vec3 vWorldPos;\n"
    "out vec4 fragColor;\n"
    "uniform vec3 uBaseColor;\n"
    "void main() {\n"
    "  vec3 dx = dFdx(vWorldPos);\n"
    "  vec3 dy = dFdy(vWorldPos);\n"
    "  vec3 normal   = normalize(cross(dx, dy));\n"
    "  vec3 lightDir = normalize(vec3(0.6, 1.0, 0.8));\n"
    "  float diff    = clamp(dot(normal, lightDir), 0.0, 1.0);\n"
    "  float ambient = 0.28;\n"
    "  float light   = ambient + (1.0 - ambient) * diff;\n"
    "  fragColor = vec4(uBaseColor * light, 1.0);\n"
    "}\n";

// 8-colour palette (BambuStudio / OrcaSlicer style)
static const QVector3D kObjColors[] = {
    QVector3D(0.949f, 0.549f, 0.220f), // orange
    QVector3D(0.231f, 0.553f, 0.867f), // blue
    QVector3D(0.298f, 0.686f, 0.455f), // green
    QVector3D(0.608f, 0.349f, 0.714f), // purple
    QVector3D(0.906f, 0.298f, 0.235f), // red
    QVector3D(0.102f, 0.737f, 0.612f), // teal
    QVector3D(0.945f, 0.769f, 0.059f), // gold
    QVector3D(0.914f, 0.118f, 0.549f), // pink
};
static constexpr int kNumColors = int(sizeof(kObjColors) / sizeof(kObjColors[0]));

// --- Geometry helpers ---
struct Vertex { float x, y, z; };

// Build K1C 220x220mm platform geometry (XZ plane, Y=0).
static QVector<Vertex> buildGeometry(
    int &axisStart,   int &axisCount,
    int &borderStart, int &borderCount,
    int &fineStart,   int &fineCount,
    int &coarseStart, int &coarseCount)
{
  QVector<Vertex> v;
  v.reserve(512);

  // Axis arrows from plate corner (0,0,0), 30mm each
  axisStart = v.size();
  v.append({0,0,0}); v.append({30, 0,  0}); // X  red
  v.append({0,0,0}); v.append({ 0,30,  0}); // Y  green
  v.append({0,0,0}); v.append({ 0, 0, 30}); // Z  blue
  axisCount = v.size() - axisStart;

  // Platform border (4 white lines)
  borderStart = v.size();
  const float P = 220.f;
  v.append({0,0,0}); v.append({P,0,0});
  v.append({P,0,0}); v.append({P,0,P});
  v.append({P,0,P}); v.append({0,0,P});
  v.append({0,0,P}); v.append({0,0,0});
  borderCount = v.size() - borderStart;

  // Fine grid 10mm step (excludes 0 and 220 which are the border)
  fineStart = v.size();
  for (float t = 10.f; t < P - 0.001f; t += 10.f)
  {
    v.append({t, 0, 0}); v.append({t, 0, P});
    v.append({0, 0, t}); v.append({P, 0, t});
  }
  fineCount = v.size() - fineStart;

  // Coarse grid 50mm step (overdrawn brighter)
  coarseStart = v.size();
  for (float t = 50.f; t < P - 0.001f; t += 50.f)
  {
    v.append({t, 0, 0}); v.append({t, 0, P});
    v.append({0, 0, t}); v.append({P, 0, t});
  }
  coarseCount = v.size() - coarseStart;

  return v;
}

// --- Constructor / Destructor ---
GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer()
{
  m_vao.destroy();
  m_vbo.destroy();
  if (!m_meshBatches.empty() && m_f)
  {
    for (auto &b : m_meshBatches)
    {
      if (b.vao) m_f->glDeleteVertexArrays(1, &b.vao);
      if (b.vbo) m_f->glDeleteBuffers(1, &b.vbo);
    }
  }
}

// --- QQuickFramebufferObject::Renderer interface ---
QOpenGLFramebufferObject *GLViewportRenderer::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4);
  m_viewSize = size;
  return new QOpenGLFramebufferObject(size, fmt);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject *item)
{
  auto *vp = static_cast<GLViewport *>(item);
  m_viewSize = QSize(static_cast<int>(vp->width()),
                     static_cast<int>(vp->height()));

  const auto events = vp->takeEvents();
  for (const auto &e : events)
  {
    switch (e.type)
    {
    case GLViewport::InputEvent::Press:
      m_dragging   = true;
      m_dragButton = e.button;
      m_lastX = e.x;
      m_lastY = e.y;
      break;
    case GLViewport::InputEvent::Release:
      m_dragging   = false;
      m_dragButton = Qt::NoButton;
      break;
    case GLViewport::InputEvent::Move:
      if (m_dragging)
      {
        const float dx = e.x - m_lastX;
        const float dy = e.y - m_lastY;
        if (m_dragButton == Qt::LeftButton)
          m_camera.orbit(dx * 0.5f, -dy * 0.5f);
        else if (m_dragButton == Qt::MiddleButton)
          m_camera.pan(dx, dy);
        m_lastX = e.x;
        m_lastY = e.y;
      }
      break;
    case GLViewport::InputEvent::Wheel:
      m_camera.zoom(e.wheelDelta);
      break;
    case GLViewport::InputEvent::FitView:
      m_camera.fitView(e.fitCX, e.fitCY, e.fitCZ, e.fitRadius);
      break;
    }
  }

  QByteArray newMesh;
  if (vp->takeMesh(newMesh, m_meshVersion))
  {
    m_pendingMesh = std::move(newMesh);
    m_meshDirty   = true;
  }
}

void GLViewportRenderer::render()
{
  if (!m_initialized)
    initialize();

  if (m_meshDirty)
  {
    uploadMesh();
    m_meshDirty = false;
  }

  m_f->glEnable(GL_DEPTH_TEST);
  m_f->glEnable(GL_CULL_FACE);
  m_f->glCullFace(GL_BACK);
  m_f->glClearColor(0.208f, 0.224f, 0.243f, 1.0f);
  m_f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = (m_viewSize.height() > 0)
                           ? float(m_viewSize.width()) / float(m_viewSize.height())
                           : 1.f;
  const QMatrix4x4 mvp = m_camera.projMatrix(aspect) * m_camera.viewMatrix();

  // 1. Draw model meshes first
  if (!m_meshBatches.empty())
  {
    m_meshProg.bind();
    m_meshProg.setUniformValue(m_uMeshMVP, mvp);

    for (auto &b : m_meshBatches)
    {
      if (b.vertexCount <= 0) continue;
      const QVector3D &col = kObjColors[b.objectId % kNumColors];
      m_meshProg.setUniformValue(m_uMeshBaseColor, col);
      m_f->glBindVertexArray(b.vao);
      m_f->glDrawArrays(GL_TRIANGLES, 0, b.vertexCount);
      m_f->glBindVertexArray(0);
    }
    m_meshProg.release();
  }

  // 2. Draw grid lines (disable back-face culling for lines)
  m_f->glDisable(GL_CULL_FACE);

  m_prog.bind();
  m_prog.setUniformValue(m_uMVP, mvp);
  m_vao.bind();

  // Fine grid (dark grey)
  m_prog.setUniformValue(m_uColor, QVector4D(0.28f, 0.30f, 0.32f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_fineGridStart, m_fineGridCount);

  // Coarse grid (medium grey)
  m_prog.setUniformValue(m_uColor, QVector4D(0.42f, 0.44f, 0.47f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_coarseGridStart, m_coarseGridCount);

  // Platform border (bright white)
  m_prog.setUniformValue(m_uColor, QVector4D(0.85f, 0.85f, 0.85f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_borderStart, m_borderCount);

  // X axis (red)
  m_prog.setUniformValue(m_uColor, QVector4D(0.85f, 0.18f, 0.18f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 0, 2);
  // Y axis (green)
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.78f, 0.22f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 2, 2);
  // Z axis (blue)
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.38f, 0.88f, 1.0f));
  m_f->glDrawArrays(GL_LINES, m_axisStart + 4, 2);

  m_vao.release();
  m_prog.release();

  m_f->glDisable(GL_DEPTH_TEST);
}

// --- Private helpers ---
void GLViewportRenderer::initialize()
{
  m_f = QOpenGLContext::currentContext()->extraFunctions();
  m_f->initializeOpenGLFunctions();

  // Compile base shaders
  m_prog.addShaderFromSourceCode(QOpenGLShader::Vertex,   kVertSrc);
  m_prog.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
  if (!m_prog.link())
    qWarning("[GLViewportRenderer] Base shader: %s", qPrintable(m_prog.log()));
  m_uMVP   = m_prog.uniformLocation("uMVP");
  m_uColor = m_prog.uniformLocation("uColor");

  // Compile mesh shader
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Vertex,   kMeshVertSrc);
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFragSrc);
  if (!m_meshProg.link())
    qWarning("[GLViewportRenderer] Mesh shader: %s", qPrintable(m_meshProg.log()));
  m_uMeshMVP       = m_meshProg.uniformLocation("uMVP");
  m_uMeshBaseColor = m_meshProg.uniformLocation("uBaseColor");

  // Upload static geometry (platform + grid + axes)
  const auto verts = buildGeometry(
      m_axisStart,   m_axisCount,
      m_borderStart, m_borderCount,
      m_fineGridStart,   m_fineGridCount,
      m_coarseGridStart, m_coarseGridCount);

  m_vao.create();
  m_vao.bind();
  m_vbo.create();
  m_vbo.bind();
  m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vbo.allocate(verts.constData(), int(verts.size() * sizeof(Vertex)));
  m_prog.enableAttributeArray(0);
  m_prog.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));
  m_vao.release();
  m_vbo.release();


  m_initialized = true;
}

void GLViewportRenderer::uploadMesh()
{
  // Free old batches
  for (auto &b : m_meshBatches)
  {
    if (b.vao) m_f->glDeleteVertexArrays(1, &b.vao);
    if (b.vbo) m_f->glDeleteBuffers(1, &b.vbo);
  }
  m_meshBatches.clear();

  if (m_pendingMesh.size() < int(sizeof(int32_t)))
    return;

  const char *p   = m_pendingMesh.constData();
  const char *end = p + m_pendingMesh.size();

  // Read objectCount from TLV header
  int32_t objCount = 0;
  memcpy(&objCount, p, sizeof(int32_t));
  p += sizeof(int32_t);

  if (objCount <= 0)
    return;

  m_meshBatches.reserve(objCount);

  for (int32_t i = 0; i < objCount; ++i)
  {
    if (p + 2 * int(sizeof(int32_t)) > end) break;

    int32_t objId    = 0;
    int32_t triCount = 0;
    memcpy(&objId,    p, sizeof(int32_t)); p += sizeof(int32_t);
    memcpy(&triCount, p, sizeof(int32_t)); p += sizeof(int32_t);

    const int dataBytes = triCount * 9 * int(sizeof(float));
    if (p + dataBytes > end) break;

    MeshBatch b;
    b.objectId    = int(objId);
    b.vertexCount = triCount * 3;

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

  int totalTris = 0;
  for (const auto &b : m_meshBatches) totalTris += b.vertexCount / 3;
  qInfo("[GLViewportRenderer] Uploaded %d batches, %d triangles",
        int(m_meshBatches.size()), totalTris);
}