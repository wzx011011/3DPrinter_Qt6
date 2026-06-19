#include "CameraServiceMock.h"

#include <QDateTime>
#include <QTimer>
#include <QUrl>
#include "core/services/CameraStream.h"

CameraServiceMock::CameraServiceMock(QObject *parent)
    : QObject(parent)
{
}

CameraServiceMock::~CameraServiceMock()
{
  // 安全停止解码线程（若正在运行），避免 QThread 析构断言
  stopRtspDecoder();
}

int CameraServiceMock::streamStatus() const
{
  return static_cast<int>(streamStatus_);
}

int CameraServiceMock::recordingStatus() const
{
  return static_cast<int>(recordingStatus_);
}

int CameraServiceMock::timelapseStatus() const
{
  return static_cast<int>(timelapseStatus_);
}

int CameraServiceMock::resolution() const
{
  return resolution_;
}

void CameraServiceMock::setResolution(int res)
{
  if (resolution_ == res)
    return;
  resolution_ = res;
  emit resolutionChanged();
}

QString CameraServiceMock::cameraUrl() const
{
  return cameraUrl_;
}

void CameraServiceMock::setCameraUrl(const QString &url)
{
  if (cameraUrl_ == url)
    return;
  cameraUrl_ = url;
  emit cameraUrlChanged();
}

QString CameraServiceMock::errorMessage() const
{
  return errorMessage_;
}

bool CameraServiceMock::cameraAvailable() const
{
  return cameraAvailable_;
}

void CameraServiceMock::startStream()
{
  if (!cameraAvailable_) {
    errorMessage_ = QStringLiteral("设备不在线或无摄像头");
    emit errorMessageChanged();
    return;
  }

  // 若用户未显式填写 URL，但选中设备有 IP，自动构造 Bambu 默认 RTSP URL。
  // 这样无需用户手动配置即可尝试拉流；若设备不支持 RTSP 会回落到 mock。
  if (cameraUrl_.isEmpty())
    cameraUrl_ = buildRtspUrl(cameraLastDeviceIp_);
  // Simulate async connection sequence: Connecting → Connected → Streaming
  streamStatus_ = CameraStreamStatus::Connecting;
  errorMessage_.clear();
  emit streamStatusChanged();
  emit errorMessageChanged();

  QTimer::singleShot(1500, this, [this]() {
    streamStatus_ = CameraStreamStatus::Connected;
    emit streamStatusChanged();

    QTimer::singleShot(800, this, [this]() {
      streamStatus_ = CameraStreamStatus::Streaming;
      emit streamStatusChanged();
      // v2.6 CAM-03：进入 Streaming 后启动底层 RTSP 解码线程。
      // 仅当 cameraUrl_ 为真实 rtsp:// 时实际拉流，否则 noop（mock）。
      startRtspDecoder();
    });
  });
}

void CameraServiceMock::stopStream()
{
  stopRtspDecoder();
  streamStatus_ = CameraStreamStatus::Disconnected;
  emit streamStatusChanged();
}

void CameraServiceMock::toggleRecording()
{
  if (streamStatus_ != CameraStreamStatus::Streaming)
    return;

  if (recordingStatus_ == CameraRecordingStatus::None
      || recordingStatus_ == CameraRecordingStatus::OffNormal
      || recordingStatus_ == CameraRecordingStatus::OffHover)
  {
    recordingStatus_ = CameraRecordingStatus::OnNormal;
  }
  else
  {
    recordingStatus_ = CameraRecordingStatus::OffNormal;
  }
  emit recordingStatusChanged();
}

void CameraServiceMock::toggleTimelapse()
{
  if (streamStatus_ != CameraStreamStatus::Streaming)
    return;

  if (timelapseStatus_ == CameraTimelapseStatus::None
      || timelapseStatus_ == CameraTimelapseStatus::OffNormal
      || timelapseStatus_ == CameraTimelapseStatus::OffHover)
  {
    timelapseStatus_ = CameraTimelapseStatus::OnNormal;
  }
  else
  {
    timelapseStatus_ = CameraTimelapseStatus::OffNormal;
  }
  emit timelapseStatusChanged();
}

void CameraServiceMock::switchCamera()
{
  // Cycle through virtual cameras (对齐 upstream CameraPopup switch_camera)
  currentCameraIndex_ = (currentCameraIndex_ + 1) % 3;
  // Reset stream when switching cameras
  if (streamStatus_ != CameraStreamStatus::Disconnected) {
    startStream();
  }
}

void CameraServiceMock::retryConnection()
{
  if (streamStatus_ == CameraStreamStatus::Error
      || streamStatus_ == CameraStreamStatus::Disconnected)
  {
    startStream();
  }
}

void CameraServiceMock::takeScreenshot()
{
  if (streamStatus_ != CameraStreamStatus::Streaming)
    return;
  // 保存当前帧（若有）为 PNG；否则只发 mock 路径（保留旧行为）
  QMutexLocker locker(&frameMutex_);
  if (!frame_.isNull()) {
    const QString path = QStringLiteral("/tmp/screenshot_") +
                         QString::number(QDateTime::currentSecsSinceEpoch()) +
                         QStringLiteral(".png");
    if (frame_.save(path, "PNG"))
      emit screenshotTaken(path);
    else
      emit screenshotTaken(QStringLiteral(""));
    return;
  }
  // Mock fallback: emit screenshot path
  emit screenshotTaken(QStringLiteral("/tmp/screenshot_") + QString::number(QDateTime::currentSecsSinceEpoch()) + QStringLiteral(".png"));
}

void CameraServiceMock::updateForDevice(const QString &deviceIp, bool online)
{
  cameraAvailable_ = online && !deviceIp.isEmpty();
  cameraLastDeviceIp_ = deviceIp;
  emit cameraAvailableChanged();

  if (!cameraAvailable_) {
    stopStream();
    recordingStatus_ = CameraRecordingStatus::None;
    timelapseStatus_ = CameraTimelapseStatus::None;
    emit recordingStatusChanged();
    emit timelapseStatusChanged();
  }
}

// ── v2.6 CAM-03: RTSP 解码集成 ──────────────────────────────────

QImage CameraServiceMock::currentFrame() const
{
  QMutexLocker locker(&frameMutex_);
  return frame_;
}

void CameraServiceMock::startRtspDecoder()
{
  // 仅当用户填写了真实 RTSP URL（或可推导出设备 IP）时才启用解码线程。
  // 否则保留 Mock 模式（仅状态机切换，不实际拉流），便于无设备环境下演示。
  QString url = cameraUrl_;
  if (!url.startsWith(QStringLiteral("rtsp://"), Qt::CaseInsensitive) &&
      !url.startsWith(QStringLiteral("rtmp://"), Qt::CaseInsensitive))
    return;

  stopRtspDecoder();
  if (!stream_)
    stream_ = new owzx::CameraStream(this);
  stream_->setUrl(url);

  // 帧到达 → 存储副本（UI 线程拉取）+ 递增 token + 广播
  connect(stream_, &owzx::CameraStream::frameReady,
          this, [this](const QImage &frame) {
            if (frame.isNull())
              return;
            {
              QMutexLocker locker(&frameMutex_);
              frame_ = frame;
              ++frameToken_;
            }
            emit frameReady(frame);
            emit frameTokenChanged();
          },
          Qt::QueuedConnection);

  connect(stream_, &owzx::CameraStream::errorOccurred,
          this, [this](const QString &err) {
            // 解码错误：仅记录，不打断状态机（状态机由 mock 控制）
            errorMessage_ = err;
            emit errorMessageChanged();
          },
          Qt::QueuedConnection);

  stream_->startStream();
}

void CameraServiceMock::stopRtspDecoder()
{
  if (stream_) {
    stream_->stopStream();
    // QThread::stopStream 内部会 quit + wait，安全删除前断开信号
    stream_->disconnect(this);
    delete stream_;
    stream_ = nullptr;
  }
  QMutexLocker locker(&frameMutex_);
  frame_ = QImage();
}

QString CameraServiceMock::buildRtspUrl(const QString &deviceIp) const
{
  if (deviceIp.isEmpty())
    return QString();
  // Bambu 打印机摄像头默认 RTSP 端点（对齐上游 MediaPlayCtrl Bambu RTSP）
  return QStringLiteral("rtsp://%1:8554/streaming/live/1").arg(deviceIp);
}
