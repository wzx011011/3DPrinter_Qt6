#include "PreviewViewModel.h"

#include "core/services/SliceService.h"
#include <QFile>
#include <QRegularExpression>
#include <QTimer>
#include <QFileInfo>
#include <QHash>
#include <cstring>

namespace
{
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

  struct FeatureStyle
  {
    QString label;
    QString color;
    float r;
    float g;
    float b;
  };

  FeatureStyle styleFor(const QString &type)
  {
    const QString t = type.toUpper();
    if (t.contains("WALL") || t.contains("PERIMETER"))
      return {QStringLiteral("外壁"), QStringLiteral("#FF8C3A"), 1.0f, 0.55f, 0.23f};
    if (t.contains("INFILL") || t.contains("FILL"))
      return {QStringLiteral("填充"), QStringLiteral("#38A4FF"), 0.22f, 0.64f, 1.0f};
    if (t.contains("SUPPORT"))
      return {QStringLiteral("支撑"), QStringLiteral("#8C63FF"), 0.55f, 0.39f, 1.0f};
    if (t.contains("TRAVEL"))
      return {QStringLiteral("空驶"), QStringLiteral("#6E7785"), 0.43f, 0.47f, 0.52f};
    return {QStringLiteral("其他"), QStringLiteral("#53D890"), 0.33f, 0.84f, 0.56f};
  }

  bool parseAxis(const QString &line, QChar axis, float &value)
  {
    const QRegularExpression re(QStringLiteral("(?:^|\\s)%1(-?\\d+(?:\\.\\d+)?)").arg(axis));
    const auto m = re.match(line);
    if (!m.hasMatch())
      return false;
    bool ok = false;
    const float v = m.captured(1).toFloat(&ok);
    if (!ok)
      return false;
    value = v;
    return true;
  }
}

PreviewViewModel::PreviewViewModel(SliceService *sliceService, QObject *parent)
    : QObject(parent), sliceService_(sliceService)
{
  playTimer_ = new QTimer(this);
  playTimer_->setInterval(24);
  connect(playTimer_, &QTimer::timeout, this, [this]()
          {
    if (moveCount_ <= 0)
      return;
    if (currentMove_ >= moveCount_)
    {
      playTimer_->stop();
      return;
    }
    currentMove_ = qMin(currentMove_ + 12, moveCount_);
    emit stateChanged(); });

  connect(sliceService_, &SliceService::progressChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::slicingChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::sliceFinished, this, [this](const QString &time)
          {
        estimatedTime_ = time;
        totalTime_ = time;
        rebuildFromGCode(sliceService_->outputPath());
        emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFailed, this, [this](const QString &)
          {
    playTimer_->stop();
    currentMove_ = 0;
    emit stateChanged(); });
}

int PreviewViewModel::progress() const
{
  return sliceService_->progress();
}

bool PreviewViewModel::slicing() const
{
  return sliceService_->slicing();
}

QString PreviewViewModel::estimatedTime() const
{
  return estimatedTime_;
}

QStringList PreviewViewModel::viewModes() const
{
  return {
      QStringLiteral("FeatureType"),
      QStringLiteral("Height"),
      QStringLiteral("Width"),
      QStringLiteral("Tool"),
      QStringLiteral("Feedrate"),
      QStringLiteral("FanSpeed"),
      QStringLiteral("Temperature"),
      QStringLiteral("ColorPrint"),
      QStringLiteral("FilamentId"),
      QStringLiteral("Chronology"),
      QStringLiteral("VolumetricRate"),
      QStringLiteral("LayerTime"),
      QStringLiteral("Shells")};
}

void PreviewViewModel::setLayerRange(int minLayer, int maxLayer)
{
  if (layerCount_ <= 0)
    return;
  const int lo = qBound(0, minLayer, layerCount_ - 1);
  const int hi = qBound(lo, maxLayer, layerCount_ - 1);
  if (lo == currentLayerMin_ && hi == currentLayerMax_)
    return;
  currentLayerMin_ = lo;
  currentLayerMax_ = hi;
  emit stateChanged();
}

void PreviewViewModel::setCurrentMove(int move)
{
  const int clamped = qBound(0, move, moveCount_);
  if (clamped == currentMove_)
    return;
  currentMove_ = clamped;
  emit stateChanged();
}

void PreviewViewModel::playAnimation()
{
  if (moveCount_ <= 0)
    return;
  playTimer_->start();
}

void PreviewViewModel::pauseAnimation()
{
  playTimer_->stop();
}

void PreviewViewModel::setViewModeIndex(int index)
{
  const int clamped = qBound(0, index, viewModes().size() - 1);
  if (clamped == viewModeIndex_)
    return;
  viewModeIndex_ = clamped;
  emit stateChanged();
}

QVariantMap PreviewViewModel::legendItem(const QString &label, const QString &color, int count) const
{
  QVariantMap item;
  item.insert(QStringLiteral("label"), label);
  item.insert(QStringLiteral("color"), color);
  item.insert(QStringLiteral("count"), count);
  return item;
}

void PreviewViewModel::rebuildFromGCode(const QString &filePath)
{
  gcodePreviewData_.clear();
  legendItems_.clear();
  layerCount_ = 0;
  moveCount_ = 0;
  currentMove_ = 0;
  currentLayerMin_ = 0;
  currentLayerMax_ = 0;
  filamentUsed_ = QStringLiteral("--");

  if (filePath.isEmpty())
    return;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  std::vector<PackedSegment> segments;
  segments.reserve(30000);

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float e = 0.f;
  int layer = 0;
  int moveIndex = 0;
  QString currentType = QStringLiteral("TRAVEL");
  QHash<QString, int> featureCount;

  while (!file.atEnd())
  {
    const QString raw = QString::fromUtf8(file.readLine()).trimmed();
    if (raw.isEmpty())
      continue;

    if (raw.startsWith(';'))
    {
      if (raw.startsWith(QStringLiteral(";TYPE:")))
        currentType = raw.mid(6).trimmed();
      continue;
    }

    if (!raw.startsWith("G0") && !raw.startsWith("G1"))
      continue;

    float nx = x;
    float ny = y;
    float nz = z;
    float ne = e;
    parseAxis(raw, 'X', nx);
    parseAxis(raw, 'Y', ny);
    parseAxis(raw, 'Z', nz);
    parseAxis(raw, 'E', ne);

    const bool moved = (nx != x) || (ny != y) || (nz != z);
    if (!moved)
    {
      x = nx;
      y = ny;
      z = nz;
      e = ne;
      continue;
    }

    if (nz > z + 0.0001f)
      ++layer;

    const bool extruding = (ne > e + 0.00001f);
    const FeatureStyle style = styleFor(extruding ? currentType : QStringLiteral("TRAVEL"));

    PackedSegment seg;
    seg.x1 = x;
    seg.y1 = y;
    seg.z1 = z;
    seg.x2 = nx;
    seg.y2 = ny;
    seg.z2 = nz;
    seg.r = style.r;
    seg.g = style.g;
    seg.b = style.b;
    seg.layer = layer;
    seg.move = moveIndex;
    segments.push_back(seg);

    featureCount[style.label] += 1;

    x = nx;
    y = ny;
    z = nz;
    e = ne;
    ++moveIndex;
  }

  moveCount_ = moveIndex;
  layerCount_ = qMax(1, layer + 1);
  currentLayerMin_ = 0;
  currentLayerMax_ = layerCount_ - 1;
  currentMove_ = moveCount_;

  if (segments.empty())
    return;

  const int count = int(segments.size());
  gcodePreviewData_.resize(8 + count * int(sizeof(PackedSegment)));
  std::memcpy(gcodePreviewData_.data(), "GCV1", 4);
  std::memcpy(gcodePreviewData_.data() + 4, &count, 4);
  std::memcpy(gcodePreviewData_.data() + 8, segments.data(), size_t(count) * sizeof(PackedSegment));

  static const QHash<QString, QString> colorMap = {
      {QStringLiteral("外壁"), QStringLiteral("#FF8C3A")},
      {QStringLiteral("填充"), QStringLiteral("#38A4FF")},
      {QStringLiteral("支撑"), QStringLiteral("#8C63FF")},
      {QStringLiteral("空驶"), QStringLiteral("#6E7785")},
      {QStringLiteral("其他"), QStringLiteral("#53D890")}};
  for (auto it = featureCount.cbegin(); it != featureCount.cend(); ++it)
    legendItems_.append(legendItem(it.key(), colorMap.value(it.key(), QStringLiteral("#53D890")), it.value()));

  filamentUsed_ = QStringLiteral("%1 segments").arg(segments.size());
}
