#include "GCodeRenderer.h"

#include "GLViewport.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QMatrix4x4>
#include <QVector4D>
#include <cstring>

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
}

void GCodeRenderer::render()
{
  initializeIfNeeded();
  if (dirty_)
    uploadIfNeeded();

  f_->glEnable(GL_DEPTH_TEST);
  f_->glClearColor(0.16f, 0.17f, 0.19f, 1.0f);
  f_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (vertices_.empty())
  {
    update();
    return;
  }

  QMatrix4x4 proj;
  const float aspect = viewSize_.height() > 0 ? float(viewSize_.width()) / float(viewSize_.height()) : 1.f;
  proj.perspective(45.f, aspect, 1.f, 5000.f);
  QMatrix4x4 view;
  view.lookAt(QVector3D(110.f, 350.f, 360.f), QVector3D(110.f, 0.f, 110.f), QVector3D(0.f, 1.f, 0.f));
  QMatrix4x4 mvp = proj * view;

  prog_.bind();
  prog_.setUniformValue(uMvp_, mvp);
  vao_.bind();

  int drawVertices = 0;
  for (size_t i = 0; i + 1 < vertices_.size(); i += 2)
  {
    const SegmentVertex &a = vertices_[i];
    if (a.layer < layerMin_ || a.layer > layerMax_)
      continue;
    if (a.move > moveEnd_)
      continue;
    drawVertices += 2;
  }

  if (drawVertices > 0)
  {
    int emitted = 0;
    std::vector<SegmentVertex> filtered;
    filtered.reserve(size_t(drawVertices));
    for (size_t i = 0; i + 1 < vertices_.size(); i += 2)
    {
      const SegmentVertex &a = vertices_[i];
      const SegmentVertex &b = vertices_[i + 1];
      if (a.layer < layerMin_ || a.layer > layerMax_)
        continue;
      if (a.move > moveEnd_)
        continue;
      filtered.push_back(a);
      filtered.push_back(b);
      emitted += 2;
    }

    vbo_.bind();
    vbo_.allocate(filtered.data(), int(filtered.size() * sizeof(SegmentVertex)));
    f_->glDrawArrays(GL_LINES, 0, emitted);
  }

  vao_.release();
  prog_.release();

  update();
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
  prog_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVert);
  prog_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFrag);
  prog_.link();
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
