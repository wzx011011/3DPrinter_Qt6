#include "FtpUploader.h"

#include <QDebug>
#include <QFileInfo>
#include <QThread>
#include <QUrl>

// libcurl C 头文件（必须先于 Qt 头文件包含以避免宏冲突，但这里 .h 已先 include Qt，
// 所以 undef 可能的冲突宏）
#include <curl/curl.h>

FtpUploader::FtpUploader(QObject *parent)
    : QObject(parent)
{
  // libcurl 全局初始化（线程安全的一次性调用）
  static bool curlInitialized = false;
  if (!curlInitialized) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curlInitialized = true;
  }
}

FtpUploader::~FtpUploader()
{
  cancel();
}

QString FtpUploader::buildFtpUrl(const QString &host, int port,
                                 const QString &accessCode,
                                 const QString &remotePath)
{
  // Bambu FTP URL：ftp://bblp:<accessCode>@<host>:<port><remotePath>
  // access code 需 URL 编码（可能含特殊字符，但 Bambu access code 是字母数字+空格）
  QString encodedCode = QString::fromUtf8(QUrl::toPercentEncoding(accessCode));
  return QStringLiteral("ftp://bblp:%1@%2:%3%4")
      .arg(encodedCode, host).arg(port).arg(remotePath);
}

int FtpUploader::progressCallback(void *clientp, double dltotal, double dlnow,
                                  double ultotal, double ulnow)
{
  auto *self = static_cast<FtpUploader *>(clientp);
  if (!self) return 0;

  if (self->cancelRequested_) return 1; // 非零返回中止传输

  if (ultotal > 0) {
    int percent = static_cast<int>((ulnow / ultotal) * 100.0);
    if (percent >= 0 && percent <= 100) {
      // 跨线程信号（QueuedConnection 由 Qt 自动处理）
      emit self->uploadProgress(percent);
    }
  }
  return 0;
}

// libcurl 读取回调（从文件读取数据上传）
static size_t readCallback(char *buffer, size_t size, size_t nitems, void *instream)
{
  auto *file = static_cast<QFile *>(instream);
  if (!file) return 0;
  return static_cast<size_t>(file->read(buffer, size * nitems));
}

bool FtpUploader::uploadFile(const QString &host, int port, const QString &accessCode,
                             const QString &localPath, const QString &remotePath)
{
  if (uploading_) {
    lastError_ = QStringLiteral("upload already in progress");
    return false;
  }

  if (!QFileInfo::exists(localPath)) {
    lastError_ = QStringLiteral("local file not found: %1").arg(localPath);
    emit uploadFinished(false, lastError_);
    return false;
  }

  uploading_ = true;
  cancelRequested_ = false;
  emit uploadStarted();

  const QString url = buildFtpUrl(host, port, accessCode, remotePath);

  // 在后台线程执行 FTPS 上传（libcurl 是阻塞 API）
  QThread *thread = QThread::create([this, url, localPath]() {
    CURL *curl = curl_easy_init();
    if (!curl) {
      QMetaObject::invokeMethod(this, [this]() {
        uploading_ = false;
        lastError_ = QStringLiteral("curl_easy_init failed");
        emit uploadFinished(false, lastError_);
      });
      return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
      curl_easy_cleanup(curl);
      QMetaObject::invokeMethod(this, [this, localPath]() {
        uploading_ = false;
        lastError_ = QStringLiteral("cannot open local file: %1").arg(localPath);
        emit uploadFinished(false, lastError_);
      });
      return;
    }

    const qint64 fileSize = file.size();

    // 设置 FTPS（隐式 TLS）
    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);           // 上传模式
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &file);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fileSize);

    // 隐式 TLS（FTPS on port 990）
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_SSL);

    // 不验证证书（打印机自签名证书）
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 超时 + 进度回调
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);       // 5 分钟超时
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // 10s 连接超时
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);

    CURLcode res = curl_easy_perform(curl);

    file.close();
    curl_easy_cleanup(curl);

    const bool ok = (res == CURLE_OK);
    const QString errorMsg = ok ? QString() :
        QStringLiteral("curl error %1: %2").arg(res).arg(QString::fromUtf8(curl_easy_strerror(res)));

    QMetaObject::invokeMethod(this, [this, ok, errorMsg]() {
      uploading_ = false;
      emit uploadFinished(ok, ok ? QString() : errorMsg);
    });
  });

  thread->setObjectName(QStringLiteral("FtpUploader"));
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  thread->start();

  qDebug("[FTP] upload started: %s -> %s", localPath.toUtf8().constData(),
         url.left(40).toUtf8().constData());
  return true;
}

void FtpUploader::cancel()
{
  if (uploading_) {
    cancelRequested_ = true;
    qDebug("[FTP] cancel requested");
  }
}
