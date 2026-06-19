#include "qml_gui/CameraImageProvider.h"

#include "core/services/CameraServiceMock.h"

CameraImageProvider::CameraImageProvider(CameraServiceMock *service)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , service_(service)
{
}

QImage CameraImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
  Q_UNUSED(id)
  if (!service_)
    return QImage();

  QImage frame = service_->currentFrame();
  if (frame.isNull())
    return QImage();

  if (size)
    *size = frame.size();

  // 若 QML 指定了 requestedSize，缩放（对齐 Image.fillMode 行为，减少 GPU 上传开销）
  if (requestedSize.isValid() && !requestedSize.isEmpty()
      && requestedSize != frame.size()) {
    return frame.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
  return frame;
}
