#include "GCodeRenderer.h"

#include "GLViewport.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QMatrix4x4>
#include <QVector4D>
#include <cstring>
#include <algorithm>

namespace
{
  const char *kVert =
      "#version 330 core\n"
      "layout(location = 0) in vec3 aPos;\n"
      "layout(location = 1) in vec3 aColor;\n"
      "uniform mat4 uMVP;\n"
      "out vec3 vColor;\n"
      "void main(){ vColor = aColor; gl_Position = uMVP * vec4(aPos,1.0); }\n";

  const char *kFrag =
      "#version 330 core\n"
      "in vec3 vColor;\n"
      "out vec4 fragColor;\n"
      "void main(){ fragColor = vec4(vColor, 1.0); }\n";

  struct PackedSegment
  {
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
    float r;
    float g;
    float b;
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float layer_time;
    float acceleration;
    int extruder_id;
    int layer;
    int move;
  };
}

GCodeRenderer::GCodeRenderer() = default;

GCodeRenderer::~GCodeRenderer()
{
  vao_.destroy();
  vbo_.destroy();
}

QOpenGLFramebufferObject *GCodeRenderer::createFramebufferObject(const QSize &size)
{
  QOpenGLFramebufferObjectFormat fmt;
  fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fmt.setSamples(4);
  viewSize_ = size;
  return new QOpenGLFramebufferObject(size, fmt);
}

void GCodeRenderer::synchronize(QQuickFramebufferObject *item)
{
  auto *vp = static_cast<GLViewport *>(item);
  viewSize_ = QSize(int(vp->width()), int(vp->height()));

  QByteArray data;
  if (vp->takePreviewData(data, dataVersion_))
  {
    pendingData_ = std::move(data);
    dirty_ = true;
  }

  layerMin_ = vp->layerMin();
  layerMax_ = vp->layerMax();
  moveEnd_ = vp->moveEnd();

  // Collect input events
  auto events = vp->takeEvents();
  for (const auto &e : events)
  {
    PendingEvent pe;
    switch (e.type)
    {
    case GLViewport::InputEvent::Press:
      pe.type = PendingEvent::Press;
      pe.button = e.button;
      pe.buttons = e.buttons;
      pe.x = e.x;
      pe.y = e.y;
      break;
    case GLViewport::InputEvent::Move:
      pe.type = PendingEvent::Move;
      pe.buttons = e.buttons;
      pe.x = e.x;
      pe.y = e.y;
      break;
    case GLViewport::InputEvent::Release:
      pe.type = PendingEvent::Release;
      pe.button = e.button;
      break;
    case GLViewport::InputEvent::Wheel:
      pe.type = PendingEvent::Wheel;
      pe.wheelDelta = e.wheelDelta;
      break;
    case GLViewport::InputEvent::FitView:
      pe.type = PendingEvent::FitView;
      pe.fitCX = e.fitCX;
      pe.fitCY = e.fitCY;
      pe.fitCZ = e.fitCZ;
      pe.fitRadius = e.fitRadius;
      break;
    case GLViewport::InputEvent::ViewPreset:
      pe.type = PendingEvent::ViewPreset;
      pe.x = e.x;
      break;
    default:
      continue;
    }
    pendingEvents_.push_back(pe);
  }
}

void GCodeRenderer::processInputEvents()
{
  for (const auto &e : pendingEvents_)
  {
    switch (e.type)
    {
    case PendingEvent::Press:
      mouseDragging_ = true;
      dragButton_ = e.button;
      lastX_ = e.x;
      lastY_ = e.y;
      break;
    case PendingEvent::Move:
    {
      const float dx = e.x - lastX_;
      const float dy = e.y - lastY_;
      if (mouseDragging_)
      {
        if (dragButton_ == Qt::LeftButton)
          camera_.orbit(dx * 0.5f, -dy * 0.5f);
        else if (dragButton_ == Qt::MiddleButton)
          camera_.pan(dx, dy);
      }
      lastX_ = e.x;
      lastY_ = e.y;
      break;
    }
    case PendingEvent::Release:
      mouseDragging_ = false;
      dragButton_ = Qt::NoButton;
      break;
    case PendingEvent::Wheel:
      camera_.zoom(e.wheelDelta);
      break;
    case PendingEvent::FitView:
      camera_.fitView(e.fitCX, e.fitCY, e.fitCZ, e.fitRadius);
      break;
    case PendingEvent::ViewPreset:
      switch (int(e.x))
      {
      case 0: camera_.viewTop(); break;
      case 1: camera_.viewFront(); break;
      case 2: camera_.viewRight(); break;
      case 3: camera_.viewIso(); break;
      default: camera_.viewIso(); break;
      }
      break;
    }
  }
  pendingEvents_.clear();
}

void GCodeRenderer::render()
{
  initializeIfNeeded();
  processInputEvents();
  if (dirty_)
    uploadIfNeeded();

  f_->glEnable(GL_DEPTH_TEST);
  f_->glClearColor(0.16f, 0.17f, 0.19f, 1.0f);
  f_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (vertices_.empty())
    return;

  const float aspect = viewSize_.height() > 0 ? float(viewSize_.width()) / float(viewSize_.height()) : 1.f;
  QMatrix4x4 mvp = camera_.projMatrix(aspect) * camera_.viewMatrix();

  prog_.bind();
  prog_.setUniformValue(uMvp_, mvp);
  vao_.bind();

  // Single-pass filter: count visible segments and build filtered buffer
  std::vector<SegmentVertex> filtered;
  filtered.reserve(vertices_.size());
  for (size_t i = 0; i + 1 < vertices_.size(); i += 2)
  {
    const SegmentVertex &a = vertices_[i];
    if (a.layer < layerMin_ || a.layer > layerMax_)
      continue;
    if (a.move > moveEnd_)
      continue;
    filtered.push_back(vertices_[i]);
    filtered.push_back(vertices_[i + 1]);
  }

  if (!filtered.empty())
  {
    vbo_.bind();
    vbo_.allocate(filtered.data(), int(filtered.size() * sizeof(SegmentVertex)));
    f_->glDrawArrays(GL_LINES, 0, int(filtered.size()));
  }

  vao_.release();
  prog_.release();
}

void GCodeRenderer::loadResult(const Slic3r::GCodeProcessorResult *result)
{
  Q_UNUSED(result);
}

void GCodeRenderer::initializeIfNeeded()
{
  if (initialized_)
    return;

  f_ = QOpenGLContext::currentContext()->extraFunctions();
  f_->initializeOpenGLFunctions();

  prog_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVert);
  prog_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFrag);
  if (!prog_.link())
    qWarning("[GCode] shader link failed: %s", qPrintable(prog_.log()));
  uMvp_ = prog_.uniformLocation("uMVP");

  vao_.create();
  vao_.bind();
  vbo_.create();
  vbo_.bind();
  vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  prog_.bind();
  f_->glEnableVertexAttribArray(0);
  f_->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SegmentVertex), reinterpret_cast<void *>(offsetof(SegmentVertex, x)));
  f_->glEnableVertexAttribArray(1);
  f_->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SegmentVertex), reinterpret_cast<void *>(offsetof(SegmentVertex, r)));
  prog_.release();

  vao_.release();
  vbo_.release();

  initialized_ = true;
}

void GCodeRenderer::uploadIfNeeded()
{
  parsePreviewData(pendingData_);
  dirty_ = false;
}

void GCodeRenderer::parsePreviewData(const QByteArray &data)
{
  vertices_.clear();
  if (data.size() < 8)
    return;

  if (std::memcmp(data.constData(), "GCV1", 4) != 0)
    return;

  int count = 0;
  std::memcpy(&count, data.constData() + 4, 4);
  if (count <= 0)
    return;

  const qsizetype payloadSize = qsizetype(count) * qsizetype(sizeof(PackedSegment));
  if (data.size() < 8 + payloadSize)
    return;

  const auto *seg = reinterpret_cast<const PackedSegment *>(data.constData() + 8);
  vertices_.reserve(size_t(count) * 2);

  // Compute bounding box for initial fitView
  float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
  float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

  for (int i = 0; i < count; ++i)
  {
    const auto &s = seg[i];
    if (s.x1 < minX) minX = s.x1; if (s.x1 > maxX) maxX = s.x1;
    if (s.x2 < minX) minX = s.x2; if (s.x2 > maxX) maxX = s.x2;
    if (s.y1 < minZ) minZ = s.y1; if (s.y1 > maxZ) maxZ = s.y1;
    if (s.y2 < minZ) minZ = s.y2; if (s.y2 > maxZ) maxZ = s.y2;
    if (s.z1 < minY) minY = s.z1; if (s.z1 > maxY) maxY = s.z1;
    if (s.z2 < minY) minY = s.z2; if (s.z2 > maxY) maxY = s.z2;
  }

  // Auto-fit camera to data bounding box
  const float cx = (minX + maxX) * 0.5f;
  const float cy = (minZ + maxZ) * 0.5f; // Y in GL = Z in GCode
  const float cz = (minY + maxY) * 0.5f; // Z in GL = Y in GCode
  const float rx = (maxX - minX) * 0.5f;
  const float ry = (maxZ - minZ) * 0.5f;
  const float rz = (maxY - minY) * 0.5f;
  const float radius = std::sqrt(rx * rx + ry * ry + rz * rz);
  if (radius > 0.001f)
    camera_.fitView(cx, cy, cz, radius);

  for (int i = 0; i < count; ++i)
  {
    SegmentVertex a;
    a.x = seg[i].x1;
    a.y = seg[i].z1;
    a.z = seg[i].y1;
    a.r = seg[i].r;
    a.g = seg[i].g;
    a.b = seg[i].b;
    a.layer = seg[i].layer;
    a.move = seg[i].move;

    SegmentVertex b = a;
    b.x = seg[i].x2;
    b.y = seg[i].z2;
    b.z = seg[i].y2;

    vertices_.push_back(a);
    vertices_.push_back(b);
  }
}
