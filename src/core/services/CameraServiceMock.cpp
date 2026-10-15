#include "CameraServiceMock.h"

#include <QDateTime>
#include <QDir>
#include <QUrl>
#include "core/services/CameraStream.h"

CameraServiceMock::CameraServiceMock(QObject *parent)
    : ICameraStreamService(parent)
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

QString CameraServiceMock::defaultRtspUrlForDevice(const QString &deviceIp)
{
  if (deviceIp.isEmpty())
    return QString();
  const QString url = QStringLiteral("rtsp://%1:8554/streaming/live/1").arg(deviceIp);
  return url;
}

void CameraServiceMock::startStream()
{
  if (!cameraAvailable_) {
    errorMessage_ = QStringLiteral("设备不在线或无摄像头");
    emit errorMessageChanged();
    return;
  }

  // 若用户未显式填写 URL，但选中设备有 IP，自动构造 Bambu 默认 RTSP URL。
  if (cameraUrl_.isEmpty())
    cameraUrl_ = buildRtspUrl(cameraLastDeviceIp_);

  // X1: 无可用摄像头源（设备无 IP 且未手填 URL）→ 显式错误披露，而非假直播。
  if (cameraUrl_.isEmpty()) {
    errorMessage_ = QStringLiteral("无可用摄像头源（RTSP URL 为空）");
    streamStatus_ = CameraStreamStatus::Error;
    emit streamStatusChanged();
    emit errorMessageChanged();
    return;
  }

  // X1: 状态由真实 RTSP 解码事件驱动——此前的 QTimer::singleShot(1500/800)
  // 无条件把状态翻成 Connected→Streaming 是假直播（连不上的打印机也显示
  // "直播中"）。现在 startStream 只进 Connecting；终态由 startRtspDecoder
  // 挂接的 CameraStream 事件（streamStarted/errorOccurred/streamStopped/
  // frameReady）决定：连不上即 Error，收到首帧才 Streaming。
  streamStatus_ = CameraStreamStatus::Connecting;
  errorMessage_.clear();
  emit streamStatusChanged();
  emit errorMessageChanged();

  startRtspDecoder();
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
  // X1: 截图路径不再写死 /tmp/（Windows 下不存在），落到系统临时目录。
  QMutexLocker locker(&frameMutex_);
  if (!frame_.isNull()) {
    const QString path = QDir::temp().filePath(QStringLiteral("owzx_screenshot_%1.png")
                                                   .arg(QDateTime::currentSecsSinceEpoch()));
    if (frame_.save(path, "PNG"))
      emit screenshotTaken(path);
    else
      emit screenshotTaken(QStringLiteral(""));
    return;
  }
  // 无帧可截：空路径即失败（QML 侧按空处理），不伪造截图成功路径。
  emit screenshotTaken(QStringLiteral(""));
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

// ── v2.6 CAM-03 / X1: RTSP 解码集成（状态事件源） ──────────────────────────

QImage CameraServiceMock::currentFrame() const
{
  QMutexLocker locker(&frameMutex_);
  return frame_;
}

void CameraServiceMock::startRtspDecoder()
{
  // X1: 非 RTSP 源（当前无真实相机后端接入的形态）→ 显式错误披露，绝不进入
  // "直播中"。此前的 noop 意味着状态机停留在假 Streaming。
  QString url = cameraUrl_;
  if (!url.startsWith(QStringLiteral("rtsp://"), Qt::CaseInsensitive) &&
      !url.startsWith(QStringLiteral("rtmp://"), Qt::CaseInsensitive))
  {
    errorMessage_ = QStringLiteral("非 RTSP 摄像头源，视频流不可用（显式演示披露）");
    streamStatus_ = CameraStreamStatus::Error;
    emit streamStatusChanged();
    emit errorMessageChanged();
    return;
  }

  stopRtspDecoder();
  if (!stream_)
    stream_ = new owzx::CameraStream(this);
  stream_->setUrl(url);

  // 帧到达 → 存储副本（UI 线程拉取）+ 递增 token + 广播；首帧翻 Streaming
  connect(stream_, &owzx::CameraStream::frameReady,
          this, [this](const QImage &frame) {
            if (frame.isNull())
              return;
            {
              QMutexLocker locker(&frameMutex_);
              frame_ = frame;
              ++frameToken_;
            }
            // X1: 首帧到达 = 真正"直播中"（Connected 是"已连接，等待视频流"）
            if (streamStatus_ == CameraStreamStatus::Connected) {
              streamStatus_ = CameraStreamStatus::Streaming;
              emit streamStatusChanged();
            }
            emit frameReady(frame);
            emit frameTokenChanged();
          },
          Qt::QueuedConnection);

  // 解码器就绪（open + codec OK）→ Connected（等待首帧）
  connect(stream_, &owzx::CameraStream::streamStarted,
          this, [this]() {
            if (streamStatus_ == CameraStreamStatus::Connecting) {
              streamStatus_ = CameraStreamStatus::Connected;
              emit streamStatusChanged();
            }
          },
          Qt::QueuedConnection);

  // X1: 打不开/流中断 → Error 终态，绝不保持"直播中"（用户已 stopStream 的
  // Disconnected 态不回翻）。
  connect(stream_, &owzx::CameraStream::errorOccurred,
          this, [this](const QString &err) {
            errorMessage_ = err;
            if (streamStatus_ == CameraStreamStatus::Connecting
                || streamStatus_ == CameraStreamStatus::Connected
                || streamStatus_ == CameraStreamStatus::Streaming) {
              streamStatus_ = CameraStreamStatus::Error;
              emit streamStatusChanged();
            }
            emit errorMessageChanged();
          },
          Qt::QueuedConnection);

  // 解码线程退出：仅当处于流活动态（Connecting/Connected/Streaming）才回
  // Disconnected——Error 是终态，等待用户 retry/stop；stopStream 已同步置
  // Disconnected，这里的兜底不重复发射无效状态。
  connect(stream_, &owzx::CameraStream::streamStopped,
          this, [this]() {
            if (streamStatus_ == CameraStreamStatus::Connecting
                || streamStatus_ == CameraStreamStatus::Connected
                || streamStatus_ == CameraStreamStatus::Streaming) {
              streamStatus_ = CameraStreamStatus::Disconnected;
              emit streamStatusChanged();
            }
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
  return defaultRtspUrlForDevice(deviceIp);
}
