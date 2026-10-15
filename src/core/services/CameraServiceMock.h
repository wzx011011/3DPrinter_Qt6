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

/// X1（审计④章）：相机流数据面窄接口（相机流角色）。
/// 刻意只收编流生命周期/状态/帧供应——录像、延时摄影、分辨率、URL 等设备
/// 对象设置面留在具体类。CameraServiceMock 与未来真实相机后端（RTSP 客户端
/// 服务 / webrtc 桥）共同实现本接口；BackendContext 是唯一装配点，测试以
/// 本接口注入替身（注入缝）。
class ICameraStreamService : public QObject
{
  Q_OBJECT

public:
  explicit ICameraStreamService(QObject *parent = nullptr) : QObject(parent) {}
  ~ICameraStreamService() override = default;

  /// 当前流连接状态（CameraStreamStatus 枚举值）
  virtual int streamStatus() const = 0;
  /// 相机对当前设备是否可用
  virtual bool cameraAvailable() const = 0;
  /// 最近一次错误（空 = 无）
  virtual QString errorMessage() const = 0;
  /// 帧令牌（每收到一帧 +1，QML 以此作 cache-buster 拉取 image://camera/live）
  virtual int frameToken() const = 0;
  /// 当前帧副本（线程安全）
  virtual QImage currentFrame() const = 0;

  /// 启动/停止流（对齐上游 MediaPlayCtrl start_stream / stop_stream）
  virtual void startStream() = 0;
  virtual void stopStream() = 0;
  /// 选中设备变化 → 更新可用性（对齐上游设备切换时的相机面刷新）
  virtual void updateForDevice(const QString &deviceIp, bool online) = 0;

signals:
  void streamStatusChanged();
  void errorMessageChanged();
  void cameraAvailableChanged();
  /// 帧令牌变化：新帧到达时发出
  void frameTokenChanged();
  /// 新帧到达（含 QImage，供直接监听者使用）
  void frameReady(const QImage &frame);
};

/// Camera service（对齐 upstream CameraPopup / MediaPlayCtrl / RTSPDecoder）
///
/// X1: 流状态由真实 RTSP 解码事件驱动（CameraStream streamStarted /
/// errorOccurred / streamStopped / frameReady）——连接不上即 Error 终态，
/// 不再有 1500/800ms 定时器无条件翻 Streaming 的假直播。真实相机后端尚未
/// 落地的形态（无 RTSP 源 / 非 RTSP URL）走显式错误披露，绝不显示"直播中"。
class CameraServiceMock final : public ICameraStreamService
{
  Q_OBJECT

  // X1: 流状态/错误/可用性/帧令牌的 QML 属性面保持原样——NOTIFY 信号由
  // ICameraStreamService 基类声明（moc 对基类信号作 NOTIFY 已验证可用）。
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
  Q_PROPERTY(int frameToken READ frameToken NOTIFY frameTokenChanged)

public:
  explicit CameraServiceMock(QObject *parent = nullptr);
  ~CameraServiceMock() override;

  // ── ICameraStreamService（相机流数据面） ──
  int streamStatus() const override;
  bool cameraAvailable() const override;
  QString errorMessage() const override;
  int frameToken() const override { return frameToken_; }
  QImage currentFrame() const override;
  void startStream() override;
  void stopStream() override;
  void updateForDevice(const QString &deviceIp, bool online) override;

  int recordingStatus() const;
  int timelapseStatus() const;
  int resolution() const;
  void setResolution(int res);
  QString cameraUrl() const;
  void setCameraUrl(const QString &url);
  static QString defaultRtspUrlForDevice(const QString &deviceIp);

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

signals:
  void recordingStatusChanged();
  void timelapseStatusChanged();
  void resolutionChanged();
  void cameraUrlChanged();
  void screenshotTaken(const QString &path);

private:
  /// 启动/停止底层 RTSP 解码线程（v2.6 CAM-03；X1 起同时是状态事件源）
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

  // v2.6 CAM-03：RTSP 解码线程（X1: 其事件驱动 streamStatus_）
  owzx::CameraStream *stream_ = nullptr;
  // v2.6 CAM-03：最新一帧（解码线程写，UI 线程读，互斥保护）
  mutable QMutex frameMutex_;
  QImage frame_;
  int frameToken_ = 0;
  // 最近一次 updateForDevice 设置的 IP（用于自动构造默认 RTSP URL）
  QString cameraLastDeviceIp_;
};
