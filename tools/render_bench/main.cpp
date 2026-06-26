#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSize>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <rhi/qshader.h>

#if QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)
#define OWZX_RENDER_BENCH_HAS_QRHI_VULKAN 1
#include <QtGui/QVulkanInstance>
#else
#define OWZX_RENDER_BENCH_HAS_QRHI_VULKAN 0
#endif

namespace {

struct RenderBenchVertex
{
  float x = 0.0f;
  float y = 0.0f;
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct RenderBenchStats
{
  int frames = 0;
  int vertices = 0;
  double uploadMs = 0.0;
  double firstFrameMs = 0.0;
  double totalMs = 0.0;
  double medianFrameMs = 0.0;
  double p95FrameMs = 0.0;
  bool initialized = false;
};

struct Options
{
  int segments = 1000000;
  int frames = 240;
  int width = 1280;
  int height = 720;
  int timeoutMs = 30000;
  QString backend = QStringLiteral("auto");
};

int valueAfter(const QStringList &args, const QString &name, int fallback)
{
  const int idx = args.indexOf(name);
  if (idx < 0 || idx + 1 >= args.size())
    return fallback;
  bool ok = false;
  const int value = args[idx + 1].toInt(&ok);
  return ok ? value : fallback;
}

Options parseOptions(const QStringList &args)
{
  Options options;
  options.segments = valueAfter(args, QStringLiteral("--segments"), options.segments);
  options.frames = valueAfter(args, QStringLiteral("--frames"), options.frames);
  options.width = valueAfter(args, QStringLiteral("--width"), options.width);
  options.height = valueAfter(args, QStringLiteral("--height"), options.height);
  options.timeoutMs = valueAfter(args, QStringLiteral("--timeout-ms"), options.timeoutMs);
  const int backendIdx = args.indexOf(QStringLiteral("--backend"));
  if (backendIdx >= 0 && backendIdx + 1 < args.size())
    options.backend = args[backendIdx + 1].toLower();
  options.segments = std::max(1, options.segments);
  options.frames = std::max(1, options.frames);
  options.width = std::max(64, options.width);
  options.height = std::max(64, options.height);
  options.timeoutMs = std::max(1000, options.timeoutMs);
  return options;
}

QVector<RenderBenchVertex> generateSegments(int segmentCount)
{
  QVector<RenderBenchVertex> vertices;
  vertices.resize(segmentCount * 2);

  const int columns = int(std::ceil(std::sqrt(double(segmentCount))));
  const float invColumns = 1.0f / float(std::max(1, columns));
  for (int i = 0; i < segmentCount; ++i) {
    const int xIndex = i % columns;
    const int yIndex = i / columns;
    const float x = -0.96f + 1.92f * float(xIndex) * invColumns;
    const float y = -0.96f + 1.92f * float(yIndex) * invColumns;
    const float hue = float(i % 1024) / 1024.0f;
    const float length = 0.0012f + 0.0018f * float((i / 17) % 11) / 10.0f;
    const float angle = float((i * 37) % 360) * 0.0174532925f;

    RenderBenchVertex a;
    a.x = x;
    a.y = y;
    a.r = 0.20f + 0.80f * hue;
    a.g = 0.85f - 0.55f * hue;
    a.b = 0.35f + 0.45f * (1.0f - hue);
    a.a = 1.0f;

    RenderBenchVertex b = a;
    b.x += std::cos(angle) * length;
    b.y += std::sin(angle) * length;

    vertices[i * 2] = a;
    vertices[i * 2 + 1] = b;
  }

  return vertices;
}

QShader loadShader(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return QShader::fromSerialized(file.readAll());
}

double percentile(QVector<qint64> values, double p)
{
  if (values.isEmpty())
    return 0.0;
  std::sort(values.begin(), values.end());
  const double pos = double(values.size() - 1) * p;
  const int last = int(values.size()) - 1;
  const int index = std::clamp(int(pos + 0.5), 0, last);
  return double(values[index]) / 1000000.0;
}

struct BackendCandidate
{
  QString name;
  QRhi::Implementation implementation = QRhi::Null;
};

QVector<BackendCandidate> backendCandidates(const QString &requested)
{
  const QVector<BackendCandidate> all = {
      {QStringLiteral("d3d12"), QRhi::D3D12},
      {QStringLiteral("vulkan"), QRhi::Vulkan},
      {QStringLiteral("d3d11"), QRhi::D3D11},
  };
  const QVector<BackendCandidate> stableAuto = {
      {QStringLiteral("d3d12"), QRhi::D3D12},
      {QStringLiteral("d3d11"), QRhi::D3D11},
  };
  if (requested == QLatin1String("auto"))
    return stableAuto;
  if (requested == QLatin1String("all") || requested == QLatin1String("compare"))
    return all;
  for (const auto &candidate : all) {
    if (candidate.name == requested)
      return {candidate};
  }
  return all;
}

struct RhiOwner
{
  std::unique_ptr<QRhi> rhi;
  QRhiD3D12InitParams d3d12Params;
  QRhiD3D11InitParams d3d11Params;
#if OWZX_RENDER_BENCH_HAS_QRHI_VULKAN
  std::unique_ptr<QVulkanInstance> vulkanInstance;
  QRhiVulkanInitParams vulkanParams;
#endif
};

std::unique_ptr<RhiOwner> createRhi(const BackendCandidate &candidate, QString *error)
{
  auto owner = std::make_unique<RhiOwner>();
  QRhiInitParams *params = nullptr;

  switch (candidate.implementation) {
    case QRhi::D3D12:
      params = &owner->d3d12Params;
      break;
    case QRhi::D3D11:
      params = &owner->d3d11Params;
      break;
    case QRhi::Vulkan:
#if OWZX_RENDER_BENCH_HAS_QRHI_VULKAN
      owner->vulkanInstance = std::make_unique<QVulkanInstance>();
      owner->vulkanInstance->setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
      if (!owner->vulkanInstance->create()) {
        *error = QStringLiteral("QVulkanInstance::create failed, errorCode=%1")
                     .arg(int(owner->vulkanInstance->errorCode()));
        return nullptr;
      }
      owner->vulkanParams.inst = owner->vulkanInstance.get();
      params = &owner->vulkanParams;
      break;
#else
      *error = QStringLiteral("QtGui was built without public Vulkan support");
      return nullptr;
#endif
    default:
      *error = QStringLiteral("unsupported backend in this benchmark build");
      return nullptr;
  }

  owner->rhi.reset(QRhi::create(candidate.implementation, params, QRhi::EnableTimestamps));
  if (!owner->rhi) {
    *error = QStringLiteral("QRhi::create failed");
    return nullptr;
  }

  return owner;
}

bool runOffscreenBenchmark(QRhi *rhi, const QVector<RenderBenchVertex> &vertices,
                           const Options &options, RenderBenchStats *stats,
                           QString *error)
{
  QShader vertexShader = loadShader(QStringLiteral(":/render_bench/shaders/segment.vert.qsb"));
  QShader fragmentShader = loadShader(QStringLiteral(":/render_bench/shaders/segment.frag.qsb"));
  if (!vertexShader.isValid() || !fragmentShader.isValid()) {
    *error = QStringLiteral("failed to load qsb shaders");
    return false;
  }

  std::unique_ptr<QRhiTexture> color(rhi->newTexture(QRhiTexture::RGBA8, QSize(options.width, options.height), 1,
                                                    QRhiTexture::RenderTarget));
  if (!color->create()) {
    *error = QStringLiteral("failed to create color target");
    return false;
  }

  std::unique_ptr<QRhiTextureRenderTarget> rt(
      rhi->newTextureRenderTarget(QRhiTextureRenderTargetDescription(QRhiColorAttachment(color.get()))));
  std::unique_ptr<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
  rt->setRenderPassDescriptor(rpDesc.get());
  if (!rt->create()) {
    *error = QStringLiteral("failed to create render target");
    return false;
  }

  std::unique_ptr<QRhiBuffer> vertexBuffer(
      rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer,
                     quint32(vertices.size() * sizeof(RenderBenchVertex))));
  if (!vertexBuffer->create()) {
    *error = QStringLiteral("failed to create vertex buffer");
    return false;
  }

  std::unique_ptr<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
  srb->setBindings({});
  if (!srb->create()) {
    *error = QStringLiteral("failed to create shader resource bindings");
    return false;
  }

  QRhiVertexInputLayout inputLayout;
  inputLayout.setBindings({QRhiVertexInputBinding(sizeof(RenderBenchVertex))});
  inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, offsetof(RenderBenchVertex, x)),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, offsetof(RenderBenchVertex, r)),
  });

  std::unique_ptr<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
  pipeline->setTopology(QRhiGraphicsPipeline::Lines);
  pipeline->setShaderStages({
      QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader),
      QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader),
  });
  pipeline->setShaderResourceBindings(srb.get());
  pipeline->setVertexInputLayout(inputLayout);
  pipeline->setRenderPassDescriptor(rpDesc.get());
  if (!pipeline->create()) {
    *error = QStringLiteral("failed to create graphics pipeline");
    return false;
  }

  QRhiCommandBuffer *cb = nullptr;
  QElapsedTimer uploadTimer;
  uploadTimer.start();
  if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess) {
    *error = QStringLiteral("beginOffscreenFrame failed for upload");
    return false;
  }
  QRhiResourceUpdateBatch *upload = rhi->nextResourceUpdateBatch();
  upload->uploadStaticBuffer(vertexBuffer.get(), vertices.constData());
  cb->resourceUpdate(upload);
  rhi->endOffscreenFrame(QRhi::SkipPresent);
  const qint64 uploadNs = uploadTimer.nsecsElapsed();

  QVector<qint64> frameTimes;
  frameTimes.reserve(options.frames);
  QElapsedTimer totalTimer;
  totalTimer.start();

  for (int frame = 0; frame < options.frames; ++frame) {
    QElapsedTimer frameTimer;
    frameTimer.start();
    if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess) {
      *error = QStringLiteral("beginOffscreenFrame failed during render");
      return false;
    }
    cb->beginPass(rt.get(), QColor(8, 11, 16, 255), {1.0f, 0});
    cb->setGraphicsPipeline(pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, float(options.width), float(options.height)));
    cb->setShaderResources(srb.get());
    const QRhiCommandBuffer::VertexInput binding(vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &binding);
    cb->draw(quint32(vertices.size()));
    cb->endPass();
    rhi->endOffscreenFrame(QRhi::SkipPresent);
    frameTimes.append(frameTimer.nsecsElapsed());
  }
  rhi->finish();

  stats->frames = options.frames;
  stats->vertices = vertices.size();
  stats->uploadMs = double(uploadNs) / 1000000.0;
  stats->firstFrameMs = frameTimes.isEmpty() ? 0.0 : double(frameTimes.front()) / 1000000.0;
  stats->totalMs = double(totalTimer.nsecsElapsed()) / 1000000.0;
  stats->medianFrameMs = percentile(frameTimes, 0.50);
  stats->p95FrameMs = percentile(frameTimes, 0.95);
  stats->initialized = true;
  return true;
}

QJsonObject statsToJson(const RenderBenchStats &stats, const Options &options, const QString &backend)
{
  QJsonObject object;
  object.insert(QStringLiteral("backend"), backend);
  object.insert(QStringLiteral("segments"), options.segments);
  object.insert(QStringLiteral("vertices"), stats.vertices);
  object.insert(QStringLiteral("frames"), stats.frames);
  object.insert(QStringLiteral("uploadMs"), stats.uploadMs);
  object.insert(QStringLiteral("firstFrameMs"), stats.firstFrameMs);
  object.insert(QStringLiteral("totalMs"), stats.totalMs);
  object.insert(QStringLiteral("medianFrameMs"), stats.medianFrameMs);
  object.insert(QStringLiteral("p95FrameMs"), stats.p95FrameMs);
  object.insert(QStringLiteral("initialized"), stats.initialized);
  return object;
}

} // namespace

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  const Options options = parseOptions(app.arguments());
  const bool collectAll = options.backend == QLatin1String("all")
      || options.backend == QLatin1String("compare");

  const QVector<RenderBenchVertex> vertices = generateSegments(options.segments);
  QJsonObject result;
  QJsonObject failures;
  QJsonArray results;

  for (const auto &candidate : backendCandidates(options.backend)) {
    QString error;
    std::unique_ptr<RhiOwner> owner = createRhi(candidate, &error);
    if (!owner) {
      failures.insert(candidate.name, error);
      continue;
    }

    RenderBenchStats stats;
    if (!runOffscreenBenchmark(owner->rhi.get(), vertices, options, &stats, &error)) {
      failures.insert(candidate.name, error);
      continue;
    }

    result = statsToJson(stats, options, candidate.name);
    result.insert(QStringLiteral("requestedBackend"), options.backend);
    if (collectAll) {
      results.append(result);
      continue;
    }
    if (!failures.isEmpty())
      result.insert(QStringLiteral("fallbackFailures"), failures);
    std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << std::endl;
    return 0;
  }

  if (collectAll) {
    result = {};
    result.insert(QStringLiteral("requestedBackend"), options.backend);
    result.insert(QStringLiteral("segments"), options.segments);
    result.insert(QStringLiteral("frames"), options.frames);
    result.insert(QStringLiteral("results"), results);
    if (!failures.isEmpty())
      result.insert(QStringLiteral("failures"), failures);
    std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << std::endl;
    return results.isEmpty() ? 1 : 0;
  }

  result.insert(QStringLiteral("error"), QStringLiteral("no QRhi backend initialized"));
  result.insert(QStringLiteral("requestedBackend"), options.backend);
  result.insert(QStringLiteral("failures"), failures);
  result.insert(QStringLiteral("segments"), options.segments);
  result.insert(QStringLiteral("frames"), options.frames);
  std::cerr << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << std::endl;
  return 1;
}
