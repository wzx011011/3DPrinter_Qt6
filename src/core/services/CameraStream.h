#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QThread>
#include <atomic>

// v2.6 CAM-01/02: 摄像头视频流（FFmpeg RTSP 解码）
// 对齐上游 wxMediaCtrl2.cpp / MediaPlayCtrl 的视频流功能
//
// 协议: RTSP/RTMP（Bambu 打印机摄像头 rtsp://<ip>/streaming/live/1）
// 解码: FFmpeg avcodec H.264 → YUV420P → RGB888 QImage
// 线程: 独立解码线程（不阻塞 UI）

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace owzx {

class CameraStream : public QThread
{
    Q_OBJECT
public:
    explicit CameraStream(QObject *parent = nullptr);
    ~CameraStream();

    /// 设置 RTSP URL（Bambu: rtsp://<ip>:8554/streaming/live/1 或 <ip>/streaming/live/1）
    void setUrl(const QString &url) { m_url = url; }
    QString url() const { return m_url; }

    /// 启动/停止解码线程
    void startStream();
    void stopStream();

    /// 状态
    bool isStreaming() const { return m_running.load(); }
    int frameWidth() const { return m_width; }
    int frameHeight() const { return m_height; }

    /// 错误信息
    QString lastError() const { return m_error; }

signals:
    /// 收到新帧（UI 线程接收，用于 VideoOutput/Image 显示）
    void frameReady(const QImage &frame);
    /// 流状态变化
    void streamStarted();
    void streamStopped();
    /// 错误
    void errorOccurred(const QString &error);

protected:
    void run() override;

private:
    QString m_url;
    QString m_error;
    std::atomic<bool> m_running{false};
    int m_width = 0;
    int m_height = 0;

    // FFmpeg 上下文（线程内使用）
    AVFormatContext *m_fmtCtx = nullptr;
    AVCodecContext *m_decCtx = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_rgbFrame = nullptr;
    SwsContext *m_swsCtx = nullptr;
    uint8_t *m_rgbBuffer = nullptr;

    bool openStream();
    void closeStream();
    bool decodeFrame();
};

} // namespace owzx
