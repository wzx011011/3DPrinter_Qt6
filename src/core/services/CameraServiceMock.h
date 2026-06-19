#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QMutex>

/// Camera status enums (对齐上游 StatusPanel CameraRecordingStatus / CameraTimelapseStatus)
enum class CameraStreamStatus {
  Disconnected,
  Connecting,
  Connected,
  Streaming,
  Error
};

namespace owzx { class CameraStream; }

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
  /// 帧令牌（每收到一帧 +1，QML Image 用作 cache-buster：image://camera/live?<token>）
  /// v2.6 CAM-03：将 CameraStream frameReady 信号转为 Q_PROPERTY 通知，QML 绑定即刷新
  Q_PROPERTY(int frameToken READ frameToken NOTIFY frameTokenChanged)

public:
  explicit CameraServiceMock(QObject *parent = nullptr);
  ~CameraServiceMock() override;

  int streamStatus() const;
  int recordingStatus() const;
  int timelapseStatus() const;
  int resolution() const;
  void setResolution(int res);
  QString cameraUrl() const;
  void setCameraUrl(const QString &url);
  QString errorMessage() const;
  bool cameraAvailable() const;

  /// 帧令牌（v2.6 CAM-03）：当前帧序号，每次新帧递增
  int frameToken() const { return frameToken_; }
  /// 取当前帧副本（线程安全，供 QQuickImageProvider 在 UI 线程拉取）
  QImage currentFrame() const;

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
  /// 帧令牌变化（v2.6 CAM-03）：新帧到达时发出
  void frameTokenChanged();
  /// 新帧到达（含 QImage 引用，供直接监听者使用）
  void frameReady(const QImage &frame);

private:
  /// 启动/停止底层 RTSP 解码线程（v2.6 CAM-03）
  void startRtspDecoder();
  void stopRtspDecoder();
  /// 计算 Bambu 打印机 RTSP URL（rtsp://<ip>:8554/streaming/live/1）
  QString buildRtspUrl(const QString &deviceIp) const;

  CameraStreamStatus streamStatus_ = CameraStreamStatus::Disconnected;
  CameraRecordingStatus recordingStatus_ = CameraRecordingStatus::None;
  CameraTimelapseStatus timelapseStatus_ = CameraTimelapseStatus::None;
  int resolution_ = 0; // 0=720P, 1=1080P
  QString cameraUrl_;
  QString errorMessage_;
  bool cameraAvailable_ = false;
  int currentCameraIndex_ = 0; // virtual camera index

  // v2.6 CAM-03：RTSP 解码线程（仅当用户填写真实 rtsp:// URL 时启用）
  owzx::CameraStream *stream_ = nullptr;
  // v2.6 CAM-03：最新一帧（解码线程写，UI 线程读，互斥保护）
  mutable QMutex frameMutex_;
  QImage frame_;
  int frameToken_ = 0;
  // 最近一次 updateForDevice 设置的 IP（用于自动构造默认 RTSP URL）
  QString cameraLastDeviceIp_;
};
