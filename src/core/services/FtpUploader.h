#pragma once

#include <QObject>
#include <QString>
#include <functional>

// v2.8 P2-C: Bambu FTPS 上传服务（libcurl 封装）。
// 上传 .gcode 文件到打印机（端口 990 隐式 TLS，用户 bblp，密码=access code）。
// 对齐上游 PrintJob::process 的 FTP 上传阶段（bambu_networking.hpp）。
//
// 用法：
//   FtpUploader uploader;
//   uploader.uploadFile("192.168.1.100", 990, "ACCESS_CODE",
//                       "/path/to/slice.gcode", "/mnt/sdcard/test.gcode",
//                       [](int percent){ ... }, [](bool ok, QString err){ ... });
//
// libcurl 在独立线程跑（避免阻塞 UI），通过信号回调进度和结果。

struct curl_ctx; // forward decl（libcurl 句柄封装，.cpp 内定义）

class FtpUploader : public QObject
{
  Q_OBJECT
public:
  explicit FtpUploader(QObject *parent = nullptr);
  ~FtpUploader() override;

  /// 上传文件到 Bambu 打印机（异步，在后台线程执行）。
  /// host: 打印机 IP；port: 通常 990（隐式 TLS）
  /// accessCode: Bambu LAN access code（FTP 密码，用户名固定 bblp）
  /// localPath: 本地 .gcode 文件路径
  /// remotePath: 打印机上的目标路径（如 /mnt/sdcard/test.gcode）
  /// 返回 true 表示上传已启动（结果通过 uploadFinished 信号通知）。
  bool uploadFile(const QString &host, int port, const QString &accessCode,
                  const QString &localPath, const QString &remotePath);

  /// 是否正在上传
  bool isUploading() const { return uploading_; }

  /// 取消上传
  void cancel();

  /// 构造 Bambu FTP URL（供测试断言，不实际连接）
  /// 格式：ftp://bblp:<accessCode>@<host>:<port><remotePath>
  static QString buildFtpUrl(const QString &host, int port,
                             const QString &accessCode,
                             const QString &remotePath);

signals:
  void uploadProgress(int percent);
  void uploadFinished(bool success, const QString &error);
  void uploadStarted();

private:
  bool uploading_ = false;
  bool cancelRequested_ = false;
  QString lastError_;

  // libcurl 进度回调（静态，转发到实例）
  static int progressCallback(void *clientp, double dltotal, double dlnow,
                              double ultotal, double ulnow);
};
