#include "GLViewportRenderer.h"
#include "GLViewport.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector>

// ── 基础着色器 (grid / axes) ─────────────────────────────────────────────────
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

// ── 网格着色器 (flat-shaded via dFdx/dFdy) ───────────────────────────────────
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
    "in vec3 vWorldPos;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  vec3 dx = dFdx(vWorldPos);\n"
    "  vec3 dy = dFdy(vWorldPos);\n"
    "  vec3 normal = normalize(cross(dx, dy));\n"
    "  vec3 lightDir = normalize(vec3(0.6, 1.0, 0.8));\n"
    "  float diff = clamp(dot(normal, lightDir), 0.0, 1.0);\n"
    "  float ambient = 0.25;\n"
    "  float light = ambient + (1.0 - ambient) * diff;\n"
    "  vec3 baseColor = vec3(0.72, 0.72, 0.74);\n" // 银灰色
    "  fragColor = vec4(baseColor * light, 1.0);\n"
    "}\n";

// ── Geometry helpers ─────────────────────────────────────────────────────────
struct Vertex
{
  float x, y, z;
};

static QVector<Vertex> buildGeometry(int &axisVerts, int &gridVerts)
{
  QVector<Vertex> v;
  v.reserve(256);

  // 3 axis lines  (origin → 100 units each)
  v.append({0, 0, 0});
  v.append({100, 0, 0}); // X
  v.append({0, 0, 0});
  v.append({0, 100, 0}); // Y
  v.append({0, 0, 0});
  v.append({0, 0, 100}); // Z
  axisVerts = v.size();  // == 6

  // Grid on XZ plane: −100…+100, step 20
  const float lo = -100.f, hi = 100.f, step = 20.f;
  for (float x = lo; x <= hi + 0.001f; x += step)
  {
    v.append({x, 0, lo});
    v.append({x, 0, hi});
  }
  for (float z = lo; z <= hi + 0.001f; z += step)
  {
    v.append({lo, 0, z});
    v.append({hi, 0, z});
  }
  gridVerts = v.size() - axisVerts;
  return v;
}

// ── Constructor / Destructor ─────────────────────────────────────────────────
GLViewportRenderer::GLViewportRenderer() = default;

GLViewportRenderer::~GLViewportRenderer()
{
  // Destroy GL objects while the context is still current
  m_vao.destroy();
  m_vbo.destroy();
  m_meshVao.destroy();
  m_meshVbo.destroy();
}

// ── QQuickFramebufferObject::Renderer interface ──────────────────────────────
QOpenGLFramebufferObject *GLViewportRenderer::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4); // MSAA ×4
  m_viewSize = size;
  return new QOpenGLFramebufferObject(size, fmt);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject *item)
{
  auto *vp = static_cast<GLViewport *>(item);

  m_viewSize = QSize(static_cast<int>(vp->width()),
                     static_cast<int>(vp->height()));

  // 取走相机输入事件
  const auto events = vp->takeEvents();
  for (const auto &e : events)
  {
    switch (e.type)
    {
    case GLViewport::InputEvent::Press:
      m_dragging = true;
      m_dragButton = e.button;
      m_lastX = e.x;
      m_lastY = e.y;
      break;
    case GLViewport::InputEvent::Release:
      m_dragging = false;
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
    }
  }

  // 取走网格数据（有新版本时）
  QByteArray newMesh;
  if (vp->takeMesh(newMesh, m_meshVersion))
  {
    m_pendingMesh = std::move(newMesh);
    m_meshDirty = true;
  }
}

void GLViewportRenderer::render()
{
  if (!m_initialized)
    initialize();

  // 若有新的网格数据，上传 GPU
  if (m_meshDirty)
  {
    uploadMesh();
    m_meshDirty = false;
  }

  m_f->glEnable(GL_DEPTH_TEST);
  m_f->glClearColor(0.208f, 0.224f, 0.243f, 1.0f);
  m_f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = (m_viewSize.height() > 0)
                           ? static_cast<float>(m_viewSize.width()) /
                                 static_cast<float>(m_viewSize.height())
                           : 1.f;

  const QMatrix4x4 mvp = m_camera.projMatrix(aspect) * m_camera.viewMatrix();

  // ── 绘制网格模型 (先画，不然网格线会覆盖模型边缘) ──
  if (m_meshVertexCount > 0)
  {
    m_meshProg.bind();
    m_meshProg.setUniformValue(m_uMeshMVP, mvp);
    m_meshVao.bind();
    m_f->glDrawArrays(GL_TRIANGLES, 0, m_meshVertexCount);
    m_meshVao.release();
    m_meshProg.release();
  }

  // ── 绘制网格线和坐标轴 ──
  m_prog.bind();
  m_prog.setUniformValue(m_uMVP, mvp);
  m_vao.bind();

  // Grid (grey)
  m_prog.setUniformValue(m_uColor, QVector4D(0.35f, 0.37f, 0.40f, 1.0f));
  m_f->glDrawArrays(GL_LINES, 6, m_totalVertices - 6);

  // X axis (red)
  m_prog.setUniformValue(m_uColor, QVector4D(0.85f, 0.18f, 0.18f, 1.0f));
  m_f->glDrawArrays(GL_LINES, 0, 2);

  // Y axis (green)
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.78f, 0.22f, 1.0f));
  m_f->glDrawArrays(GL_LINES, 2, 2);

  // Z axis (blue)
  m_prog.setUniformValue(m_uColor, QVector4D(0.18f, 0.38f, 0.88f, 1.0f));
  m_f->glDrawArrays(GL_LINES, 4, 2);

  m_vao.release();
  m_prog.release();

  m_f->glDisable(GL_DEPTH_TEST);
}

// ── Private helpers ──────────────────────────────────────────────────────────
void GLViewportRenderer::initialize()
{
  m_f = QOpenGLContext::currentContext()->functions();
  m_f->initializeOpenGLFunctions();

  // 编译基础着色器 (grid/axes)
  m_prog.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertSrc);
  m_prog.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
  if (!m_prog.link())
    qWarning("[GLViewportRenderer] Base shader link error: %s",
             qPrintable(m_prog.log()));

  m_uMVP = m_prog.uniformLocation("uMVP");
  m_uColor = m_prog.uniformLocation("uColor");

  // 上传 grid + axes 静态几何
  int axisVerts = 0, gridVerts = 0;
  const auto verts = buildGeometry(axisVerts, gridVerts);
  m_totalVertices = verts.size();

  m_vao.create();
  m_vao.bind();
  m_vbo.create();
  m_vbo.bind();
  m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vbo.allocate(verts.constData(),
                 static_cast<int>(verts.size() * sizeof(Vertex)));
  m_prog.enableAttributeArray(0);
  m_prog.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));
  m_vao.release();
  m_vbo.release();

  // 编译网格着色器 (flat-shaded)
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Vertex, kMeshVertSrc);
  m_meshProg.addShaderFromSourceCode(QOpenGLShader::Fragment, kMeshFragSrc);
  if (!m_meshProg.link())
    qWarning("[GLViewportRenderer] Mesh shader link error: %s",
             qPrintable(m_meshProg.log()));

  m_uMeshMVP = m_meshProg.uniformLocation("uMVP");

  // 创建网格 VAO/VBO（内容稍后 uploadMesh() 填充）
  m_meshVao.create();
  m_meshVbo.create();

  m_initialized = true;
}

void GLViewportRenderer::uploadMesh()
{
  if (!m_meshVao.isCreated())
    return;

  const int floatCount = m_pendingMesh.size() / static_cast<int>(sizeof(float));
  m_meshVertexCount = floatCount / 3; // 每个顶点 3 floats

  if (m_meshVertexCount == 0)
    return;

  m_meshProg.bind();
  m_meshVao.bind();
  m_meshVbo.bind();
  m_meshVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  m_meshVbo.allocate(m_pendingMesh.constData(), m_pendingMesh.size());

  m_meshProg.enableAttributeArray(0);
  m_meshProg.setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));

  m_meshVao.release();
  m_meshVbo.release();
  m_meshProg.release();

  qInfo("[GLViewportRenderer] Mesh uploaded: %d triangles", m_meshVertexCount / 3);
}
