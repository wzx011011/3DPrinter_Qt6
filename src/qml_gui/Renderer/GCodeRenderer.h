#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QByteArray>
#include <QVector3D>
#include <vector>

class GLViewport;
namespace Slic3r
{
  struct GCodeProcessorResult;
}

class GCodeRenderer : public QQuickFramebufferObject::Renderer
{
public:
  GCodeRenderer();
  ~GCodeRenderer() override;

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
  void synchronize(QQuickFramebufferObject *item) override;
  void render() override;

  void loadResult(const Slic3r::GCodeProcessorResult *result);

private:
  struct SegmentVertex
  {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float r = 1.f;
    float g = 1.f;
    float b = 1.f;
    int layer = 0;
    int move = 0;
  };

  void initializeIfNeeded();
  void uploadIfNeeded();
  void parsePreviewData(const QByteArray &data);

  bool initialized_ = false;
  bool dirty_ = false;
  QSize viewSize_;

  QOpenGLExtraFunctions *f_ = nullptr;
  QOpenGLShaderProgram prog_;
  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
  int uMvp_ = -1;

  std::vector<SegmentVertex> vertices_;
  QByteArray pendingData_;
  int dataVersion_ = -1;

  int layerMin_ = 0;
  int layerMax_ = 0;
  int moveEnd_ = 0;
};
