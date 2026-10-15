#ifdef OWZX_HAS_FFMPEG
// FFmpeg C 接口（必须先于 Qt 头文件 include，避免宏冲突）
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

// FFmpeg 可能污染的宏清理（slots/emit/signals/keywords）
#undef slots
#undef signals
#undef emit
#undef foreach
#endif

#include "CameraStream.h"
#include <QDebug>
#include <QImage>

namespace owzx {

CameraStream::CameraStream(QObject *parent)
    : QThread(parent)
{
}

CameraStream::~CameraStream()
{
    stopStream();
    closeStream();
}

void CameraStream::startStream()
{
    if (m_running.load()) return;
    if (m_url.isEmpty()) {
        emit errorOccurred("No RTSP URL set");
        return;
    }

    m_running.store(true);
    start(LowPriority);  // QThread::run 在低优先级
    qDebug("[Camera] starting stream: %s", m_url.toUtf8().constData());
}

void CameraStream::stopStream()
{
    // X1: 协作取消——置停标志后，解码线程内的 FFmpeg 阻塞点经
    // interrupt_callback（openStream 里挂到 AVFormatContext）立即以错误返回，
    // wait() 在秒级内自然汇合。不再 terminate()：强杀持 FFmpeg 内部锁的线程
    // 会撕裂解码器状态并可能死锁在下次 av_* 调用上。
    m_running.store(false);
    if (isRunning()) {
        if (!wait(3000))
            qWarning("[Camera] decode thread still running after 3s "
                     "(interrupt callback should have aborted blocking IO)");
    }
    emit streamStopped();
}

int CameraStream::onInterruptCallback(void *ctx)
{
    auto *self = static_cast<CameraStream *>(ctx);
    if (!self)
        return 1;
    // 返回 1 = 中止当前阻塞的 avformat_open_input / av_read_frame
    if (!self->m_running.load())
        return 1;
    // 打开阶段硬截止：连不上即错误态（s 6s 确定性上界），不再依赖协议层
    // 超时选项的版本语义
    if (self->m_opening.load() && self->m_openTimer.elapsed() > kOpenTimeoutMs) {
        self->m_openDeadlineExceeded = true;
        return 1;
    }
    return 0;
}

bool CameraStream::openStream()
{
#ifndef OWZX_HAS_FFMPEG
    m_error = "FFmpeg support is not available in this build";
    return false;
#else
    // X1: 先分配 context 挂 interrupt callback（协作取消 + 打开截止），再打开
    // 输入——avformat_open_input 接受调用方预分配的 context 并沿用其回调。
    m_fmtCtx = avformat_alloc_context();
    if (!m_fmtCtx) {
        m_error = "avformat_alloc_context failed";
        return false;
    }
    m_fmtCtx->interrupt_callback.callback = &CameraStream::onInterruptCallback;
    m_fmtCtx->interrupt_callback.opaque = this;
    m_openDeadlineExceeded = false;
    m_openTimer.start();
    m_opening.store(true);

    // 打开 RTSP 流（低延迟模式）
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);      // TCP 传输（比 UDP 稳定）
    av_dict_set(&opts, "stimeout", "5000000", 0);         // 5 秒 socket IO 超时（FFmpeg ≤4.x 键名）
    av_dict_set(&opts, "timeout", "5000000", 0);          // FFmpeg 5.0+ 更名（≤4.x 下未知键被安全忽略）
    av_dict_set(&opts, "fflags", "nobuffer", 0);          // 无缓冲（低延迟）
    av_dict_set(&opts, "flags", "low_delay", 0);

    int ret = avformat_open_input(&m_fmtCtx, m_url.toUtf8().constData(), nullptr, &opts);
    av_dict_free(&opts);

    if (ret != 0) {
        m_opening.store(false);
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        m_error = m_openDeadlineExceeded
                      ? QString("RTSP connect/open timeout after %1 ms").arg(kOpenTimeoutMs)
                      : QString("avformat_open_input failed: %1").arg(errbuf);
        return false;
    }

    ret = avformat_find_stream_info(m_fmtCtx, nullptr);
    m_opening.store(false);
    if (ret < 0) {
        m_error = m_openDeadlineExceeded
                      ? QString("RTSP probe timeout after %1 ms").arg(kOpenTimeoutMs)
                      : "avformat_find_stream_info failed";
        return false;
    }

    // 找视频流
    int videoStreamIdx = -1;
    const AVCodec *decoder = nullptr;
    videoStreamIdx = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (videoStreamIdx < 0 || !decoder) {
        m_error = "No video stream found";
        return false;
    }

    // 打开解码器
    m_decCtx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(m_decCtx, m_fmtCtx->streams[videoStreamIdx]->codecpar);

    ret = avcodec_open2(m_decCtx, decoder, nullptr);
    if (ret < 0) {
        m_error = "avcodec_open2 failed";
        return false;
    }

    m_width = m_decCtx->width;
    m_height = m_decCtx->height;

    // 分配帧
    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();

    // RGB 转换上下文（YUV420P → RGB888）
    m_swsCtx = sws_getContext(m_width, m_height, m_decCtx->pix_fmt,
                               m_width, m_height, AV_PIX_FMT_RGB24,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx) {
        m_error = "sws_getContext failed";
        return false;
    }

    // RGB 缓冲区
    int rgbSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_width, m_height, 1);
    m_rgbBuffer = (uint8_t *)av_malloc(rgbSize);
    av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_rgbBuffer,
                         AV_PIX_FMT_RGB24, m_width, m_height, 1);

    qDebug("[Camera] stream opened: %dx%d", m_width, m_height);
    return true;
#endif
}

void CameraStream::closeStream()
{
#ifdef OWZX_HAS_FFMPEG
    if (m_swsCtx) { sws_freeContext(m_swsCtx); m_swsCtx = nullptr; }
    if (m_rgbBuffer) { av_free(m_rgbBuffer); m_rgbBuffer = nullptr; }
    if (m_rgbFrame) { av_frame_free(&m_rgbFrame); }
    if (m_frame) { av_frame_free(&m_frame); }
    if (m_decCtx) { avcodec_free_context(&m_decCtx); }
    if (m_fmtCtx) { avformat_close_input(&m_fmtCtx); }
#else
    m_fmtCtx = nullptr;
    m_decCtx = nullptr;
    m_frame = nullptr;
    m_rgbFrame = nullptr;
    m_swsCtx = nullptr;
    m_rgbBuffer = nullptr;
#endif
}

bool CameraStream::decodeFrame()
{
#ifndef OWZX_HAS_FFMPEG
    return false;
#else
    AVPacket *pkt = av_packet_alloc();
    bool gotFrame = false;

    while (m_running.load() && av_read_frame(m_fmtCtx, pkt) >= 0) {
        if (avcodec_send_packet(m_decCtx, pkt) >= 0) {
            while (avcodec_receive_frame(m_decCtx, m_frame) >= 0) {
                // YUV → RGB
                sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_height,
                          m_rgbFrame->data, m_rgbFrame->linesize);

                // RGB → QImage。X1: 具名常量 QImage::Format_RGB888（packed 3 字节
                // RGB，与 sws 输出 AV_PIX_FMT_RGB24 逐字节对应，行距即
                // linesize[0]=width*3）。原写法 static_cast<QImage::Format>(4)
                // 实为 Format_RGB32（4 字节/像素）——按错误布局解释 RGB24 缓冲，
                // 颜色与行对齐均是错的；魔数注释"Format_RGB24 = 4"本身也不存在
                // 该枚举名。
                QImage img(m_rgbBuffer, m_width, m_height, m_rgbFrame->linesize[0],
                           QImage::Format_RGB888);

                // 复制 QImage（深拷贝，因为 m_rgbBuffer 会被下一帧覆盖）
                emit frameReady(img.copy());

                gotFrame = true;
                break;
            }
        }
        av_packet_unref(pkt);
        if (gotFrame) break;
    }

    av_packet_free(&pkt);
    return gotFrame;
#endif
}

void CameraStream::run()
{
    if (!openStream()) {
        emit errorOccurred(m_error);
        m_running.store(false);
        return;
    }

    emit streamStarted();

    // 解码循环
    while (m_running.load()) {
        if (!decodeFrame()) {
            // 读取失败（可能流结束或断开）
            qWarning("[Camera] decodeFrame failed, stream may have ended");
            break;
        }
    }

    closeStream();
    m_running.store(false);
    emit streamStopped();
    qDebug("[Camera] stream thread exited");
}

} // namespace owzx
