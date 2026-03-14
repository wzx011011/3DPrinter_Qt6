#pragma once

#include <QObject>
#include <QString>

/// Camera status enums (对齐上游 StatusPanel CameraRecordingStatus / CameraTimelapseStatus)
enum class CameraStreamStatus {
  Disconnected,
  Connecting,
  Connected,
  Streaming,
  Error
};

enum class CameraRecordingStatus {
  None,
  OffNormal,
  OffHover,
  OnNormal,
  OnHover
};

enum class CameraTimelapseStatus {
  None,
  OffNormal,
  OffHover,
  OnNormal,
  OnHover
};

/// Mock camera service (对齐 upstream CameraPopup / MediaPlayCtrl / RTSPDecoder)
///
/// In Mock mode, this simulates camera stream state transitions without
/// actual RTSP/WebRTC/FFmpeg dependencies. Real implementation would require:
///   - Qt Multimedia (QMediaPlayer + QVideoSink) or
///   - FFmpeg integration via RTSPDecoder.h / WebRTCDecoder.h
class CameraServiceMock final : public QObject
{
  Q_OBJECT

  /// Current stream connection state
  Q_PROPERTY(int streamStatus READ streamStatus NOTIFY streamStatusChanged)
  /// Whether camera recording is active
  Q_PROPERTY(int recordingStatus READ recordingStatus NOTIFY recordingStatusChanged)
  /// Whether timelapse capture is active
  Q_PROPERTY(int timelapseStatus READ timelapseStatus NOTIFY timelapseStatusChanged)
  /// Video resolution (0=720P, 1=1080P)
  Q_PROPERTY(int resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
  /// Custom camera URL (RTSP/MJPEG)
  Q_PROPERTY(QString cameraUrl READ cameraUrl WRITE setCameraUrl NOTIFY cameraUrlChanged)
  /// Stream error message
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
  /// Whether camera is available for the selected device
  Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY cameraAvailableChanged)

public:
  explicit CameraServiceMock(QObject *parent = nullptr);

  int streamStatus() const;
  int recordingStatus() const;
  int timelapseStatus() const;
  int resolution() const;
  void setResolution(int res);
  QString cameraUrl() const;
  void setCameraUrl(const QString &url);
  QString errorMessage() const;
  bool cameraAvailable() const;

  /// Start camera stream connection (对齐 upstream MediaPlayCtrl start_stream)
  Q_INVOKABLE void startStream();
  /// Stop camera stream (对齐 upstream MediaPlayCtrl stop_stream)
  Q_INVOKABLE void stopStream();
  /// Toggle camera recording (对齐 upstream StatusPanel toggle_recording)
  Q_INVOKABLE void toggleRecording();
  /// Toggle timelapse capture (对齐 upstream StatusPanel toggle_timelapse)
  Q_INVOKABLE void toggleTimelapse();
  /// Switch to next virtual camera (对齐 upstream CameraPopup switch_camera)
  Q_INVOKABLE void switchCamera();
  /// Retry failed connection (对齐 upstream CameraPopup retry)
  Q_INVOKABLE void retryConnection();
  /// Take screenshot (对齐 upstream CameraPopup screenshot)
  Q_INVOKABLE void takeScreenshot();
  /// Update camera availability for the current device
  Q_INVOKABLE void updateForDevice(const QString &deviceIp, bool online);

signals:
  void streamStatusChanged();
  void recordingStatusChanged();
  void timelapseStatusChanged();
  void resolutionChanged();
  void cameraUrlChanged();
  void errorMessageChanged();
  void cameraAvailableChanged();
  void screenshotTaken(const QString &path);

private:
  CameraStreamStatus streamStatus_ = CameraStreamStatus::Disconnected;
  CameraRecordingStatus recordingStatus_ = CameraRecordingStatus::None;
  CameraTimelapseStatus timelapseStatus_ = CameraTimelapseStatus::None;
  int resolution_ = 0; // 0=720P, 1=1080P
  QString cameraUrl_;
  QString errorMessage_;
  bool cameraAvailable_ = false;
  int currentCameraIndex_ = 0; // virtual camera index
};
