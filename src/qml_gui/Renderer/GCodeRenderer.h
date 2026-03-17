#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QByteArray>
#include <QVector3D>
#include <vector>
#include "CameraController.h"

class GLViewport;
#ifdef HAS_LIBSLIC3R
namespace Slic3r
{
  struct GCodeProcessorResult;
}
#endif

class GCodeRenderer : public QQuickFramebufferObject::Renderer
{
public:
  GCodeRenderer();
  ~GCodeRenderer() override;

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
  void synchronize(QQuickFramebufferObject *item) override;
  void render() override;

#ifdef HAS_LIBSLIC3R
  void loadResult(const Slic3r::GCodeProcessorResult *result);
#endif

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
  void processInputEvents();
  void initializeMarker();

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
  /// 对齐上游 GCodeViewer m_travel_visibility
  bool showTravelMoves_ = true;
  /// 对齐上游 GCodeViewer show_bed
  bool showBed_ = true;

  /// 3D Marker（对齐上游 GCodeViewer::Marker，使用 gouraud_light 风格渲染）
  bool showMarker_ = true;
  float markerX_ = 0.f, markerY_ = 0.f, markerZ_ = 0.f;
  QOpenGLShaderProgram markerProg_;
  QOpenGLVertexArrayObject markerVao_;
  QOpenGLBuffer markerVbo_{QOpenGLBuffer::VertexBuffer};
  int markerVertexCount_ = 0;
  int markerU_mvp_ = -1;
  int markerU_model_ = -1;
  int markerU_color_ = -1;
  bool markerInitialized_ = false;

  // Camera controls
  CameraController camera_;
  bool mouseDragging_ = false;
  Qt::MouseButton dragButton_ = Qt::NoButton;
  float lastX_ = 0.f;
  float lastY_ = 0.f;

  // Pending input events (consumed in render thread)
  struct PendingEvent
  {
    enum Type { Press, Move, Release, Wheel, FitView, ViewPreset };
    Type type;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons;
    float x = 0.f;
    float y = 0.f;
    float wheelDelta = 0.f;
    float fitCX = 0.f, fitCY = 0.f, fitCZ = 0.f, fitRadius = 0.f;
  };
  std::vector<PendingEvent> pendingEvents_;
};
