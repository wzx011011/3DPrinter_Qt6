#include "GLViewportRenderer.h"
#include "GLViewport.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector>

// ── GLSL shaders ────────────────────────────────────────────────────────────
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
}

void GLViewportRenderer::render()
{
  if (!m_initialized)
    initialize();

  m_f->glEnable(GL_DEPTH_TEST);
  m_f->glClearColor(0.208f, 0.224f, 0.243f, 1.0f); // matches #353941
  m_f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float aspect = (m_viewSize.height() > 0)
                           ? static_cast<float>(m_viewSize.width()) /
                                 static_cast<float>(m_viewSize.height())
                           : 1.f;

  const QMatrix4x4 mvp = m_camera.projMatrix(aspect) * m_camera.viewMatrix();

  m_prog.bind();
  m_prog.setUniformValue(m_uMVP, mvp);
  m_vao.bind();

  // Grid (grey) — vertices start at index 6
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

  // Restore default Qt Quick state
  m_f->glDisable(GL_DEPTH_TEST);
}

// ── Private helpers ──────────────────────────────────────────────────────────
void GLViewportRenderer::initialize()
{
  m_f = QOpenGLContext::currentContext()->functions();
  m_f->initializeOpenGLFunctions();

  // Compile + link shaders
  m_prog.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertSrc);
  m_prog.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc);
  if (!m_prog.link())
    qWarning("[GLViewportRenderer] Shader link error: %s",
             qPrintable(m_prog.log()));

  m_uMVP = m_prog.uniformLocation("uMVP");
  m_uColor = m_prog.uniformLocation("uColor");

  // Upload geometry
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

  // Vertex attribute 0: position (3 floats)
  m_prog.enableAttributeArray(0);
  m_prog.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));

  m_vao.release();
  m_vbo.release();

  m_initialized = true;
}
