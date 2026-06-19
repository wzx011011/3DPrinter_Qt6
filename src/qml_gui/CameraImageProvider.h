#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QPointer>

class CameraServiceMock;

/// v2.6 CAM-03: QQuickImageProvider — 将 CameraServiceMock.currentFrame()
/// 暴露为 image://camera/live，供 QML Image 组件绑定。
///
/// QML 端用法（带 cache-buster，每帧 token 变化时强制重新拉取）：
///   Image { source: "image://camera/live?" + monitorVm.cameraFrameToken }
///
/// 线程模型：QQuickImageProvider::requestImage 在 ImageReader 线程调用，
/// CameraServiceMock::currentFrame() 内部用 QMutex 保护，线程安全。
class CameraImageProvider final : public QQuickImageProvider
{
public:
  explicit CameraImageProvider(CameraServiceMock *service);

  /// 返回当前帧（深拷贝）。id/query 忽略（仅靠 cache-buster 触发刷新）。
  QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
  QPointer<CameraServiceMock> service_;
};
