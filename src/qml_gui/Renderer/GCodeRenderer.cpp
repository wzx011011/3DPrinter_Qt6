#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GCodeRenderer.h"

#include "GLViewport.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QMatrix4x4>
#include <QVector4D>
#include <cstring>
#include <cfloat>
#include <cmath>
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

  // 3D Marker shader（对齐上游 gouraud_light）
  const char *kMarkerVert =
      "#version 330 core\n"
      "layout(location = 0) in vec3 aPos;\n"
      "layout(location = 1) in vec3 aNormal;\n"
      "uniform mat4 uMVP;\n"
      "uniform mat4 uModel;\n"
      "out vec3 vNormal;\n"
      "out vec3 vWorldPos;\n"
      "void main(){\n"
      "  vec4 wp = uModel * vec4(aPos, 1.0);\n"
      "  vWorldPos = wp.xyz;\n"
      "  vNormal = mat3(uModel) * aNormal;\n"
      "  gl_Position = uMVP * wp;\n"
      "}\n";

  const char *kMarkerFrag =
      "#version 330 core\n"
      "in vec3 vNormal;\n"
      "in vec3 vWorldPos;\n"
      "out vec4 fragColor;\n"
      "uniform vec4 uColor;\n"
      "void main(){\n"
      "  vec3 n = normalize(vNormal);\n"
      "  // Two directional lights (对齐上游 gouraud_light: top + front)\n"
      "  vec3 lightDir1 = normalize(vec3(0.0, 1.0, 0.3));\n"
      "  vec3 lightDir2 = normalize(vec3(0.0, 0.0, 1.0));\n"
      "  float diff1 = max(dot(n, lightDir1), 0.0);\n"
      "  float diff2 = max(dot(n, lightDir2), 0.0);\n"
      "  float ambient = 0.3;\n"
      "  float light = ambient + diff1 * 0.5 + diff2 * 0.3;\n"
      "  fragColor = vec4(uColor.rgb * light, uColor.a);\n"
      "}\n";

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
  markerVao_.destroy();
  markerVbo_.destroy();
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
  showTravelMoves_ = vp->showTravelMoves();
  showBed_ = vp->showBed();
  showMarker_ = vp->showMarker();
  markerX_ = vp->markerX();
  markerY_ = vp->markerY();
  markerZ_ = vp->markerZ();

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

  // --- 热床网格（对齐上游 GCodeViewer show_bed）---
  if (showBed_)
  {
    /// 热床网格：覆盖 -150..150 范围，间距 10mm，颜色 #2a3545
    static const float kBedExtent = 150.f;
    static const float kBedStep = 10.f;
    static const float kBedR = 0.165f;  // #2a3545
    static const float kBedG = 0.208f;
    static const float kBedB = 0.271f;

    std::vector<SegmentVertex> gridVerts;
    const int lineCount = int(kBedExtent / kBedStep);
    gridVerts.reserve(size_t(lineCount * 2 + lineCount * 2) * 2);
    for (int i = -lineCount; i <= lineCount; ++i)
    {
      const float coord = float(i) * kBedStep;
      // X 方向线（沿 X 轴，Y 方向偏移）
      gridVerts.push_back({-kBedExtent, 0.f, coord, kBedR, kBedG, kBedB, 0, 0});
      gridVerts.push_back({ kBedExtent, 0.f, coord, kBedR, kBedG, kBedB, 0, 0});
      // Y 方向线（沿 Z 轴，X 方向偏移）
      gridVerts.push_back({coord, 0.f, -kBedExtent, kBedR, kBedG, kBedB, 0, 0});
      gridVerts.push_back({coord, 0.f,  kBedExtent, kBedR, kBedG, kBedB, 0, 0});
    }
    if (!gridVerts.empty())
    {
      vbo_.bind();
      vbo_.allocate(gridVerts.data(), int(gridVerts.size() * sizeof(SegmentVertex)));
      f_->glDrawArrays(GL_LINES, 0, int(gridVerts.size()));
    }
  }

  // Single-pass filter: count visible segments and build filtered buffer
  /// 过滤逻辑：层范围、移动范围、空驶移动（对齐上游 m_travel_visibility）
  std::vector<SegmentVertex> filtered;
  filtered.reserve(vertices_.size());
  for (size_t i = 0; i + 1 < vertices_.size(); i += 2)
  {
    const SegmentVertex &a = vertices_[i];
    if (a.layer < layerMin_ || a.layer > layerMax_)
      continue;
    if (a.move > moveEnd_)
      continue;
    /// 空驶移动颜色：r ~0.43, g ~0.47, b ~0.52（上游 TRAVEL 标识色）
    if (!showTravelMoves_
        && a.r >= 0.40f && a.r <= 0.46f
        && a.g >= 0.44f && a.g <= 0.50f
        && a.b >= 0.49f && a.b <= 0.55f)
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

  // --- 3D Marker（对齐上游 GCodeViewer::Marker，gouraud_light 风格 3D 箭头）---
  if (showMarker_ && markerX_ != 0.f && markerY_ != 0.f)
  {
    initializeMarker();

    QMatrix4x4 model;
    // G-code → GL 坐标转换（对齐 parsePreviewData 中的 y↔z 互换）
    model.translate(markerX_, markerZ_, markerY_);
    // Scale arrow to reasonable size in mm
    float scale = 1.5f;
    model.scale(scale, scale, scale);

    markerProg_.bind();
    markerProg_.setUniformValue(markerU_mvp_, mvp);
    markerProg_.setUniformValue(markerU_model_, model);
    // Semi-transparent white (对齐上游 m_model.set_color({ 1.0f, 1.0f, 1.0f, 0.5f }))
    markerProg_.setUniformValue(markerU_color_, QVector4D(1.0f, 1.0f, 1.0f, 0.75f));

    f_->glEnable(GL_BLEND);
    f_->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f_->glDepthMask(GL_FALSE);

    markerVao_.bind();
    f_->glDrawArrays(GL_TRIANGLES, 0, markerVertexCount_);
    markerVao_.release();

    f_->glDepthMask(GL_TRUE);
    f_->glDisable(GL_BLEND);
    markerProg_.release();
  }
}

void GCodeRenderer::initializeMarker()
{
  if (markerInitialized_)
    return;

  markerProg_.addShaderFromSourceCode(QOpenGLShader::Vertex, kMarkerVert);
  markerProg_.addShaderFromSourceCode(QOpenGLShader::Fragment, kMarkerFrag);
  if (!markerProg_.link())
    qWarning("[GCode] marker shader link failed: %s", qPrintable(markerProg_.log()));
  markerU_mvp_ = markerProg_.uniformLocation("uMVP");
  markerU_model_ = markerProg_.uniformLocation("uModel");
  markerU_color_ = markerProg_.uniformLocation("uColor");

  // Generate arrow geometry (对齐上游 stilized_arrow)
  // Cone + cylinder: simple stylized arrow pointing up (Z axis)
  struct Vert { float x, y, z, nx, ny, nz; };
  std::vector<Vert> verts;

  const float coneRadius = 1.0f;
  const float coneHeight = 2.5f;
  const float cylRadius = 0.35f;
  const float cylHeight = 2.0f;
  const int segments = 16;

  auto pushTri = [&](const Vert &a, const Vert &b, const Vert &c) {
    verts.push_back(a); verts.push_back(b); verts.push_back(c);
  };

  // Cone (top part)
  for (int i = 0; i < segments; ++i) {
    float a1 = 2.0f * float(M_PI) * float(i) / segments;
    float a2 = 2.0f * float(M_PI) * float(i + 1) / segments;
    float x1 = coneRadius * std::cos(a1), z1 = coneRadius * std::sin(a1);
    float x2 = coneRadius * std::cos(a2), z2 = coneRadius * std::sin(a2);
    // Normal for cone surface (angled outward)
    float ny = coneRadius / coneHeight;
    float nl = std::sqrt(1.0f + ny * ny);
    // Base of cone is at cylHeight
    Vert tip = {0.f, cylHeight + coneHeight, 0.f, 0.f, 1.f, 0.f};
    Vert b1 = {x1, cylHeight, z1, x1/nl, ny/nl, z1/nl};
    Vert b2 = {x2, cylHeight, z2, x2/nl, ny/nl, z2/nl};
    pushTri(tip, b1, b2);
  }

  // Cylinder (bottom part)
  for (int i = 0; i < segments; ++i) {
    float a1 = 2.0f * float(M_PI) * float(i) / segments;
    float a2 = 2.0f * float(M_PI) * float(i + 1) / segments;
    float x1 = cylRadius * std::cos(a1), z1 = cylRadius * std::sin(a1);
    float x2 = cylRadius * std::cos(a2), z2 = cylRadius * std::sin(a2);
    // Side normals
    float nl = 1.0f / cylRadius;
    Vert a1t = {x1, cylHeight, z1, x1*nl, 0.f, z1*nl};
    Vert a2t = {x2, cylHeight, z2, x2*nl, 0.f, z2*nl};
    Vert a1b = {x1, 0.f, z1, x1*nl, 0.f, z1*nl};
    Vert a2b = {x2, 0.f, z2, x2*nl, 0.f, z2*nl};
    pushTri(a1t, a1b, a2b);
    pushTri(a1t, a2b, a2t);
    // Bottom cap
    Vert c1 = {x1, 0.f, z1, 0.f, -1.f, 0.f};
    Vert c2 = {x2, 0.f, z2, 0.f, -1.f, 0.f};
    Vert cc = {0.f, 0.f, 0.f, 0.f, -1.f, 0.f};
    pushTri(cc, c1, c2);
  }

  markerVertexCount_ = int(verts.size());

  markerVao_.create();
  markerVao_.bind();
  markerVbo_.create();
  markerVbo_.bind();
  markerVbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
  markerVbo_.allocate(verts.data(), int(verts.size() * sizeof(Vert)));
  f_->glEnableVertexAttribArray(0);
  f_->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), reinterpret_cast<void *>(0));
  f_->glEnableVertexAttribArray(1);
  f_->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), reinterpret_cast<void *>(3 * sizeof(float)));
  markerVao_.release();
  markerVbo_.release();

  markerInitialized_ = true;
}

#ifdef HAS_LIBSLIC3R
void GCodeRenderer::loadResult(const Slic3r::GCodeProcessorResult *result)
{
  Q_UNUSED(result);
}
#endif

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
