#include "CameraServiceMock.h"

#include <QDateTime>
#include <QTimer>

CameraServiceMock::CameraServiceMock(QObject *parent)
    : QObject(parent)
{
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
    });
  });
}

void CameraServiceMock::stopStream()
{
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
  // Mock: emit screenshot path
  emit screenshotTaken(QStringLiteral("/tmp/screenshot_") + QString::number(QDateTime::currentSecsSinceEpoch()) + QStringLiteral(".png"));
}

void CameraServiceMock::updateForDevice(const QString &deviceIp, bool online)
{
  cameraAvailable_ = online && !deviceIp.isEmpty();
  emit cameraAvailableChanged();

  if (!cameraAvailable_) {
    stopStream();
    recordingStatus_ = CameraRecordingStatus::None;
    timelapseStatus_ = CameraTimelapseStatus::None;
    emit recordingStatusChanged();
    emit timelapseStatusChanged();
  }
}
