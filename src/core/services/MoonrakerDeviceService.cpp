#include "MoonrakerDeviceService.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

namespace {
const char kSettingsGroup[] = "Devices/Lan";
const char kSettingsLastIp[] = "Devices/LanLastIp";
constexpr int kMoonrakerDefaultPort = 7125;
constexpr int kMoonrakerPollMs = 3000;
constexpr int kLegacyWsPort = 9999;
} // namespace

// ── 构造/析构 ─────────────────────────────────────────────────────────────

MoonrakerDeviceService::MoonrakerDeviceService(QObject *parent)
    : IDeviceDataPlane(parent),
      mdns_(new MdnsDiscovery(this)),
      probe_(new LanDeviceProbe(this)),
      manager_(new QNetworkAccessManager(this)),
      moonrakerPoller_(new QTimer(this))
{
    loadPersisted();

    moonrakerPoller_->setInterval(kMoonrakerPollMs);
    connect(moonrakerPoller_, &QTimer::timeout, this,
            &MoonrakerDeviceService::pollMoonrakerStatus);

    connect(mdns_, &MdnsDiscovery::deviceFound, this,
            &MoonrakerDeviceService::onMdnsDeviceFound);
    connect(mdns_, &MdnsDiscovery::scanningChanged, this, [this]() {
        if (scanning_ != mdns_->scanning()) {
            scanning_ = mdns_->scanning();
            emit devicesChanged();
        }
    });

    // ws_ 惰性创建（openStatusChannel 时）——QWebSocket 构造即初始化 TLS，
    // 测试二进制里会触发 paho-mqtt/系统 libcrypto 符号冲突段错误。
}

MoonrakerDeviceService::~MoonrakerDeviceService()
{
    moonrakerPoller_->stop();
    if (ws_)
        ws_->close();
}

// ── 设备记录辅助 ──────────────────────────────────────────────────────────

int MoonrakerDeviceService::matchingIndexByIp(const QString &ip) const
{
    for (int i = 0; i < devices_.size(); ++i)
        if (devices_.at(i).ip == ip)
            return i;
    return -1;
}

MoonrakerDeviceService::Device *MoonrakerDeviceService::selected()
{
    if (selectedIndex_ < 0 || selectedIndex_ >= devices_.size())
        return nullptr;
    return &devices_[selectedIndex_];
}

const MoonrakerDeviceService::Device *MoonrakerDeviceService::selected() const
{
    if (selectedIndex_ < 0 || selectedIndex_ >= devices_.size())
        return nullptr;
    return &devices_.at(selectedIndex_);
}

int MoonrakerDeviceService::filteredToReal(int filteredIndex) const
{
    if (filteredIndex < 0)
        return -1;
    int seen = -1;
    for (int i = 0; i < devices_.size(); ++i) {
        const Device &dev = devices_.at(i);
        if (!searchText_.isEmpty()
            && !(dev.name.contains(searchText_, Qt::CaseInsensitive)
                 || dev.model.contains(searchText_, Qt::CaseInsensitive)
                 || dev.ip.contains(searchText_)))
            continue;
        ++seen;
        if (seen == filteredIndex)
            return i;
    }
    return -1;
}

QString MoonrakerDeviceService::moonrakerBase(const Device &dev) const
{
    const int port = dev.moonrakerPort > 0 ? dev.moonrakerPort : kMoonrakerDefaultPort;
    return QStringLiteral("http://%1:%2").arg(dev.ip).arg(port);
}

void MoonrakerDeviceService::touch()
{
    emit devicesChanged();
    emit selectedDeviceChanged();
}

// ── 持久化（QSettings Devices/Lan JSON） ──────────────────────────────────

void MoonrakerDeviceService::loadPersisted()
{
    QSettings settings;
    const QString json = settings.value(QLatin1String(kSettingsGroup)).toString();
    if (json.isEmpty())
        return;
    const QJsonArray arr = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Device dev;
        dev.name = o.value(QStringLiteral("name")).toString();
        dev.model = o.value(QStringLiteral("model")).toString();
        dev.ip = o.value(QStringLiteral("ip")).toString();
        dev.mac = o.value(QStringLiteral("mac")).toString();
        dev.httpPort = o.value(QStringLiteral("httpPort")).toInt(80);
        dev.secure = o.value(QStringLiteral("secure")).toBool(false);
        dev.wssPort = o.value(QStringLiteral("wssPort")).toInt();
        dev.isMoonraker = o.value(QStringLiteral("isMoonraker")).toBool(false);
        dev.moonrakerPort = o.value(QStringLiteral("moonrakerPort")).toInt();
        dev.oldPrinter = o.value(QStringLiteral("oldPrinter")).toBool(false);
        if (dev.ip.isEmpty())
            continue;
        devices_.append(dev);
    }
    // 启动时自动重连上次选中设备（连接在 UI 侧触发，避免后台自发建连）。
    const QString lastIp = settings.value(QLatin1String(kSettingsLastIp)).toString();
    if (!lastIp.isEmpty())
        selectedIndex_ = matchingIndexByIp(lastIp);
}

void MoonrakerDeviceService::persist() const
{
    QJsonArray arr;
    for (const Device &dev : devices_) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), dev.name);
        o.insert(QStringLiteral("model"), dev.model);
        o.insert(QStringLiteral("ip"), dev.ip);
        o.insert(QStringLiteral("mac"), dev.mac);
        o.insert(QStringLiteral("httpPort"), dev.httpPort);
        o.insert(QStringLiteral("secure"), dev.secure);
        o.insert(QStringLiteral("wssPort"), dev.wssPort);
        o.insert(QStringLiteral("isMoonraker"), dev.isMoonraker);
        o.insert(QStringLiteral("moonrakerPort"), dev.moonrakerPort);
        o.insert(QStringLiteral("oldPrinter"), dev.oldPrinter);
        arr.append(o);
    }
    QSettings settings;
    settings.setValue(QLatin1String(kSettingsGroup),
                      QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

// ── 发现 ─────────────────────────────────────────────────────────────────

void MoonrakerDeviceService::scanDevices()
{
    if (scanning_)
        return;
    seenInstances_.clear();
    // mDNS 发现（官方 cxmdns 对齐）：_creality 前缀 + 服务名内编码端口；
    // 每个候选再过 /info 探测补全机型字段。跨网段/老机型由手动添加覆盖。
    mdns_->start(3000);
}

void MoonrakerDeviceService::onMdnsDeviceFound(const QVariantMap &device)
{
    const QString ip = device.value(QStringLiteral("ip")).toString();
    if (matchingIndexByIp(ip) != -1)
        return;
    // 记录 mDNS 先行信息：typeCode "00"=Moonraker + 端口（/info 不含这两个
    // 字段，探测完成时合并进设备记录）。
    QVariantMap info;
    info.insert(QStringLiteral("isMoonraker"),
                device.value(QStringLiteral("isMoonraker")).toBool());
    info.insert(QStringLiteral("moonrakerPort"),
                device.value(QStringLiteral("moonrakerPort")).toInt());
    pendingMdns_.insert(ip, info);
    probeAndAdd(ip, QString());
}

void MoonrakerDeviceService::probeAndAdd(const QString &ip, const QString &nameHint)
{
    auto *probe = new LanDeviceProbe(this);
    probe->setHttpsDisabled(probeHttpsDisabled_);
    connect(probe, &LanDeviceProbe::probeFinished, this,
            [this, probe, nameHint](const QVariantMap &map) {
                probe->deleteLater();
                if (!map.value(QStringLiteral("ok")).toBool())
                    return; // 探测失败不收录（发现列表只保留活设备）
                const QString probedIp = map.value(QStringLiteral("address")).toString();
                if (matchingIndexByIp(probedIp) != -1)
                    return;
                Device dev;
                dev.model = map.value(QStringLiteral("model")).toString();
                dev.name = nameHint.isEmpty() ? dev.model : nameHint;
                dev.ip = probedIp;
                dev.mac = map.value(QStringLiteral("mac")).toString();
                dev.secure = map.value(QStringLiteral("secureConnection")).toBool();
                dev.wssPort = map.value(QStringLiteral("wssPort")).toInt();
                dev.httpPort = dev.secure ? 443 : 80;
                dev.oldPrinter = map.value(QStringLiteral("oldPrinter")).toBool();
                // 合并 mDNS 先行信息（Moonraker 标记 + 端口）
                if (pendingMdns_.contains(dev.ip)) {
                    const QVariantMap mdns = pendingMdns_.take(dev.ip);
                    dev.isMoonraker = mdns.value(QStringLiteral("isMoonraker")).toBool();
                    dev.moonrakerPort = mdns.value(QStringLiteral("moonrakerPort")).toInt();
                }
                devices_.append(dev);
                persist();
                touch();
            });
    probe->probe(ip);
}

// ── 设备清单 ─────────────────────────────────────────────────────────────

int MoonrakerDeviceService::deviceCount() const
{
    if (searchText_.isEmpty())
        return devices_.size();
    int n = 0;
    for (const Device &dev : devices_) {
        if (dev.name.contains(searchText_, Qt::CaseInsensitive)
            || dev.model.contains(searchText_, Qt::CaseInsensitive)
            || dev.ip.contains(searchText_))
            ++n;
    }
    return n;
}

QVariantMap MoonrakerDeviceService::deviceAt(int filteredIndex) const
{
    QVariantMap map;
    const int real = filteredToReal(filteredIndex);
    if (real == -1)
        return map;
    const Device &dev = devices_.at(real);
    map.insert(QStringLiteral("name"), dev.name);
    map.insert(QStringLiteral("model"), dev.model);
    map.insert(QStringLiteral("ip"), dev.ip);
    map.insert(QStringLiteral("mac"), dev.mac);
    map.insert(QStringLiteral("status"), dev.status);
    map.insert(QStringLiteral("online"), dev.online);
    map.insert(QStringLiteral("demo"), false); // LAN 后端无演示条目
    map.insert(QStringLiteral("isMoonraker"), dev.isMoonraker);
    map.insert(QStringLiteral("oldPrinter"), dev.oldPrinter);
    map.insert(QStringLiteral("progress"), dev.progress);
    map.insert(QStringLiteral("taskName"), dev.taskName);
    return map;
}

void MoonrakerDeviceService::selectDevice(int filteredIndex)
{
    const int real = filteredToReal(filteredIndex);
    if (real == -1 || real == selectedIndex_)
        return;
    selectedIndex_ = real;
    QSettings settings;
    settings.setValue(QLatin1String(kSettingsLastIp), devices_.at(real).ip);
    emit selectedDeviceChanged();
}

bool MoonrakerDeviceService::addManualDevice(const QString &name, const QString &ip,
                                             const QString &accessCode, int port)
{
    Q_UNUSED(accessCode);
    Q_UNUSED(port); // Moonraker/4408 无 Bambu 访问码语义
    if (ip.isEmpty() || matchingIndexByIp(ip) != -1)
        return false;
    Device dev;
    dev.name = name.isEmpty() ? ip : name;
    dev.ip = ip;
    dev.status = QStringLiteral("connecting");
    devices_.append(dev);
    persist();
    touch();
    // 手动添加先入表（connecting），探测补全机型字段后刷新。
    probeAndAdd(ip, name);
    return true;
}

QString MoonrakerDeviceService::deviceListJson() const
{
    QJsonArray arr;
    for (int i = 0; i < devices_.size(); ++i)
        arr.append(QJsonValue::fromVariant(deviceAt(i)));
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

int MoonrakerDeviceService::filteredDeviceCount() const { return deviceCount(); }

QString MoonrakerDeviceService::searchText() const { return searchText_; }

void MoonrakerDeviceService::setSearchText(const QString &text)
{
    if (searchText_ == text)
        return;
    searchText_ = text;
    touch();
}

// ── 状态通道 ─────────────────────────────────────────────────────────────

void MoonrakerDeviceService::connectDevice(int filteredIndex)
{
    selectDevice(filteredIndex);
    openStatusChannel();
}

void MoonrakerDeviceService::disconnectDevice(int)
{
    closeStatusChannel();
    if (Device *dev = selected()) {
        dev->online = false;
        dev->status = QStringLiteral("offline");
        touch();
    }
}

void MoonrakerDeviceService::openStatusChannel()
{
    Device *dev = selected();
    if (!dev || dev->ip.isEmpty())
        return;
    closeStatusChannel();
    dev->status = QStringLiteral("connecting");
    touch();

    if (dev->isMoonraker) {
        // Moonraker 机型：HTTP 轮询 /printer/objects/query（无 ws）
        moonrakerPoller_->start();
        pollMoonrakerStatus();
        return;
    }
    // 4408 机型：ws://IP:9999（加密 → wss://IP:wssPort）
    const int port = portsOverrideWs_ > 0
                         ? portsOverrideWs_
                         : (dev->secure ? (dev->wssPort > 0 ? dev->wssPort : 443)
                                        : kLegacyWsPort);
    const QString scheme = dev->secure ? QStringLiteral("wss") : QStringLiteral("ws");
    ws_->open(QUrl(QStringLiteral("%1://%2:%3").arg(scheme).arg(dev->ip).arg(port)));
}

void MoonrakerDeviceService::closeStatusChannel()
{
    moonrakerPoller_->stop();
    if (ws_)
        ws_->close();
}

void MoonrakerDeviceService::pollMoonrakerStatus()
{
    Device *dev = selected();
    if (!dev || !dev->isMoonraker)
        return;
    const QString devIp = dev->ip;
    QNetworkRequest request{
        QUrl(moonrakerBase(*dev)
             + QStringLiteral("/printer/objects/query?"
                              "extruder=target,temperature&heater_bed=target,"
                              "temperature&print_stats=filename,state&display_status="
                              "progress"))};
    request.setTransferTimeout(2500);
    QNetworkReply *reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, devIp]() {
        reply->deleteLater();
        const int idx = matchingIndexByIp(devIp);
        if (idx == -1 || reply->error() != QNetworkReply::NoError)
            return;
        const QJsonObject status =
            QJsonDocument::fromJson(reply->readAll())
                .object()
                .value(QStringLiteral("result"))
                .toObject()
                .value(QStringLiteral("status"))
                .toObject();
        Device &dev = devices_[idx];
        dev.online = true;
        const QJsonObject extruder = status.value(QStringLiteral("extruder")).toObject();
        dev.nozzleTemp = int(extruder.value(QStringLiteral("temperature")).toDouble());
        dev.nozzleTarget = int(extruder.value(QStringLiteral("target")).toDouble());
        const QJsonObject bed = status.value(QStringLiteral("heater_bed")).toObject();
        dev.bedTemp = int(bed.value(QStringLiteral("temperature")).toDouble());
        dev.bedTarget = int(bed.value(QStringLiteral("target")).toDouble());
        const QJsonObject stats = status.value(QStringLiteral("print_stats")).toObject();
        dev.taskName = stats.value(QStringLiteral("filename")).toString();
        const QString state = stats.value(QStringLiteral("state")).toString();
        dev.status = (state == QLatin1String("printing")) ? QStringLiteral("printing")
                    : (state == QLatin1String("paused"))  ? QStringLiteral("paused")
                                                          : QStringLiteral("idle");
        dev.progress = int(status.value(QStringLiteral("display_status"))
                               .toObject()
                               .value(QStringLiteral("progress"))
                               .toDouble(0)
                           * 100);
        touch();
    });
}

void MoonrakerDeviceService::onWsTextMessage(const QString &message)
{
    // 4408 推送：状态字段在顶层 params 风格 JSON 里
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return;
    const QJsonObject root = doc.object();
    // 收尾#4：pFileList 推送（requestDeviceFiles 的 get 应答）
    if (root.contains(QStringLiteral("pFileList"))) {
        QVariantList files;
        const QJsonArray arr = root.value(QStringLiteral("pFileList")).toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject f = v.toObject();
            QVariantMap row;
            row.insert(QStringLiteral("name"), f.value(QStringLiteral("name")).toString());
            row.insert(QStringLiteral("size"), f.value(QStringLiteral("size")).toVariant());
            row.insert(QStringLiteral("path"), f.value(QStringLiteral("path")).toString());
            files.append(row);
        }
        emit deviceFilesReady(files);
    }
    if (Device *dev = selected()) {
        applyStatusFields(*dev, root);
        touch();
    }
}

void MoonrakerDeviceService::applyStatusFields(Device &dev, const QJsonObject &params)
{
    dev.online = true;
    // 收尾#1：err.errcode（0/缺失=无错误；bundle deviceMgr 同款字段）
    const QJsonObject err = params.value(QStringLiteral("err")).toObject();
    if (!err.isEmpty())
        dev.errorCode = err.value(QStringLiteral("errcode")).toInt(0);
    const int state = params.value(QStringLiteral("state")).toInt(-1);
    if (state == 0)
        dev.status = QStringLiteral("idle");
    else if (state == 1)
        dev.status = QStringLiteral("printing");
    const QJsonObject printInfo = params.value(QStringLiteral("printInfo")).toObject();
    if (printInfo.contains(QStringLiteral("gcodeName")))
        dev.taskName = printInfo.value(QStringLiteral("gcodeName")).toString();
    const QJsonArray telemetry = params.value(QStringLiteral("iotTelemetry")).toArray();
    for (const QJsonValue &v : telemetry) {
        const QJsonObject t = v.toObject();
        const QString key = t.value(QStringLiteral("key")).toString();
        const int value = int(t.value(QStringLiteral("value")).toDouble());
        if (key == QLatin1String("printProgress"))
            dev.progress = value;
        else if (key == QLatin1String("printLeftTime"))
            dev.remainingMin = value;
        else if (key == QLatin1String("layer"))
            dev.currentLayer = value;
    }
    const QJsonObject currentTemp = params.value(QStringLiteral("currentTemp")).toObject();
    if (!currentTemp.isEmpty()) {
        dev.nozzleTemp = int(currentTemp.value(QStringLiteral("nozzleTemp")).toDouble());
        dev.bedTemp = int(currentTemp.value(QStringLiteral("bedTemp")).toDouble());
    }
    const QJsonObject targetTemp = params.value(QStringLiteral("targetTemp")).toObject();
    if (!targetTemp.isEmpty()) {
        dev.nozzleTarget = int(targetTemp.value(QStringLiteral("nozzleTemp")).toDouble());
        dev.bedTarget = int(targetTemp.value(QStringLiteral("bedTemp")).toDouble());
    }
}

// ── 控制命令 ─────────────────────────────────────────────────────────────

void MoonrakerDeviceService::wsSet(const QJsonObject &params)
{
    // 未连接：无副作用（诚实不支持）
    if (!ws_ || ws_->state() != QAbstractSocket::ConnectedState)
        return;
    QJsonObject msg;
    msg.insert(QStringLiteral("method"), QStringLiteral("set"));
    msg.insert(QStringLiteral("params"), params);
    ws_->sendTextMessage(QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
}

void MoonrakerDeviceService::pausePrint(int)
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        manager_->post(QNetworkRequest(
                           QUrl(moonrakerBase(*dev) + QStringLiteral("/printer/print/pause"))),
                       QByteArray());
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("pause"), 1);
    wsSet(params);
}

void MoonrakerDeviceService::resumePrint(int)
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        manager_->post(QNetworkRequest(
                           QUrl(moonrakerBase(*dev) + QStringLiteral("/printer/print/resume"))),
                       QByteArray());
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("pause"), 0);
    wsSet(params);
}

void MoonrakerDeviceService::stopPrint(int)
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        manager_->post(QNetworkRequest(
                           QUrl(moonrakerBase(*dev) + QStringLiteral("/printer/print/cancel"))),
                       QByteArray());
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("stop"), 1);
    wsSet(params);
}

// ── 上传 + 启动（sendPrintViaFtp 接口名的 Moonraker/4408 实现） ──────────

bool MoonrakerDeviceService::uploadAndStart(int filteredIndex, const QString &gcodePath)
{
    const int real = filteredToReal(filteredIndex);
    if (real == -1) {
        emit sendPrintFailed(QStringLiteral("未选择打印机"));
        return false;
    }
    if (uploadInFlight_) {
        emit sendPrintFailed(QStringLiteral("已有发送任务进行中"));
        return false;
    }
    const Device dev = devices_.at(real);
    auto *file = new QFile(gcodePath);
    if (!file->open(QIODevice::ReadOnly)) {
        const QString p = gcodePath;
        delete file;
        emit sendPrintFailed(QStringLiteral("无法读取 G-code 文件：%1").arg(p));
        return false;
    }
    const QString fileName = QFileInfo(gcodePath).fileName();

    uploadInFlight_ = true;
    emit sendPrintUploadStarted();

    // 4408: POST /upload/<urlencode(name)>；Moonraker: POST /server/files/upload
    const int port = portsOverrideHttp_ > 0
                         ? portsOverrideHttp_
                         : (dev.isMoonraker
                                ? (dev.moonrakerPort > 0 ? dev.moonrakerPort
                                                         : kMoonrakerDefaultPort)
                                : dev.httpPort);
    QUrl url = dev.isMoonraker
                   ? QUrl(moonrakerBase(dev) + QStringLiteral("/server/files/upload"))
                   : QUrl(QStringLiteral("http://%1:%2/upload/%3")
                              .arg(dev.ip)
                              .arg(port)
                              .arg(QString::fromUtf8(QUrl::toPercentEncoding(fileName))));

    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral(
                                    "form-data; name=\"file\"; filename=\"%1\"")
                                    .arg(fileName)));
    filePart.setBodyDevice(file);
    file->setParent(multi);
    multi->append(filePart);

    QNetworkRequest request{url};
    QNetworkReply *reply = manager_->post(request, multi);
    uploadReply_ = reply; // 收尾#3：取消句柄

    connect(reply, &QNetworkReply::uploadProgress, this,
            [this](qint64 sent, qint64 total) {
                if (total > 0)
                    emit sendPrintUploadProgress(int(sent * 100 / total));
            });
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileName, dev]() {
        reply->deleteLater();
        uploadReply_ = nullptr;
        uploadInFlight_ = false;
        const bool wasAborted = reply->error() == QNetworkReply::OperationCanceledError;
        if (wasAborted) {
            emit sendPrintCancelled();
            return;
        }
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit sendPrintFailed(reply->errorString());
            return;
        }
        if (dev.isMoonraker) {
            // Moonraker 上传成功（{"result":"ok"}）→ 启动打印。
            QNetworkRequest startReq{
                QUrl(moonrakerBase(dev)
                     + QStringLiteral("/printer/print/start?filename=")
                     + QString::fromUtf8(QUrl::toPercentEncoding(fileName)))};
            QNetworkReply *startReply = manager_->post(startReq, QByteArray());
            connect(startReply, &QNetworkReply::finished, this,
                    [this, startReply]() {
                        startReply->deleteLater();
                        if (startReply->error() == QNetworkReply::NoError)
                            emit sendPrintSucceeded();
                        else
                            emit sendPrintFailed(startReply->errorString());
                    });
            return;
        }
        // 4408：响应 body 含 "OK" 即成功；启动走 ws opGcodeFile。
        if (!body.contains("OK")) {
            emit sendPrintFailed(QStringLiteral("设备上传响应异常"));
            return;
        }
        QJsonObject params;
        params.insert(QStringLiteral("opGcodeFile"),
                      QStringLiteral("printprt:/") + fileName);
        if (enableSelfTest_)
            params.insert(QStringLiteral("enableSelfTest"), 1);
        wsSet(params);
        emit sendPrintSucceeded();
    });
    return true;
}

bool MoonrakerDeviceService::sendPrintViaFtp(int filteredIndex, const QString &gcodePath)
{
    // 接口名保留（IDeviceDataPlane 兼容），实现为 Moonraker/4408 HTTP 上传。
    return uploadAndStart(filteredIndex, gcodePath);
}

void MoonrakerDeviceService::startPrint(int filteredIndex, const QString &gcodePath)
{
    uploadAndStart(filteredIndex, gcodePath);
}

void MoonrakerDeviceService::cancelSendPrint()
{
    // 收尾#3：abort() → OperationCanceledError → finished 分支发
    // sendPrintCancelled（与 mock 后端语义一致）。
    if (uploadReply_ && uploadInFlight_)
        uploadReply_->abort();
}

// ── 遥测（Moonraker 后端无 Bambu MQTT 信封——诚实 false） ────────────────

bool MoonrakerDeviceService::applyMqttReportPayload(const QString &, int)
{
    return false;
}

// ── 便捷 getter（扩展面覆盖） ─────────────────────────────────────────────

QString MoonrakerDeviceService::selectedDeviceName() const
{
    const Device *dev = selected();
    return dev ? dev->name : QString();
}

QString MoonrakerDeviceService::selectedDeviceModel() const
{
    const Device *dev = selected();
    return dev ? dev->model : QString();
}

QString MoonrakerDeviceService::selectedDeviceSerial() const
{
    const Device *dev = selected();
    return dev ? dev->mac : QString(); // LAN 设备无序列号，mac 即身份
}

int MoonrakerDeviceService::selectedDeviceProgress() const
{
    const Device *dev = selected();
    return dev ? dev->progress : 0;
}

QString MoonrakerDeviceService::selectedDeviceTaskName() const
{
    const Device *dev = selected();
    return dev ? dev->taskName : QString();
}

// ── 遥测 getter（选中设备） ───────────────────────────────────────────────

bool MoonrakerDeviceService::selectedDeviceOnline() const
{
    const Device *dev = selected();
    return dev ? dev->online : false;
}

QString MoonrakerDeviceService::selectedDeviceStatus() const
{
    const Device *dev = selected();
    return dev ? dev->status : QString();
}

QString MoonrakerDeviceService::selectedDeviceIp() const
{
    const Device *dev = selected();
    return dev ? dev->ip : QString();
}

bool MoonrakerDeviceService::isMqttConnected() const
{
    // 语义映射：LAN 后端"已连接" = 状态通道活着（ws 连接或轮询进行中）
    const Device *dev = selected();
    return dev ? dev->online : false;
}

int MoonrakerDeviceService::selectedDeviceTemperature() const
{
    const Device *dev = selected();
    return dev ? dev->nozzleTemp : 0;
}

int MoonrakerDeviceService::selectedDeviceBedTemperature() const
{
    const Device *dev = selected();
    return dev ? dev->bedTemp : 0;
}

int MoonrakerDeviceService::selectedDeviceNozzleTargetTemp() const
{
    const Device *dev = selected();
    return dev ? dev->nozzleTarget : 0;
}

int MoonrakerDeviceService::selectedDeviceBedTargetTemp() const
{
    const Device *dev = selected();
    return dev ? dev->bedTarget : 0;
}

int MoonrakerDeviceService::selectedDeviceTotalLayerNum() const
{
    const Device *dev = selected();
    return dev ? dev->totalLayer : 0;
}

int MoonrakerDeviceService::selectedDeviceCurrentLayerNum() const
{
    const Device *dev = selected();
    return dev ? dev->currentLayer : 0;
}

int MoonrakerDeviceService::selectedDeviceRemainingTime() const
{
    const Device *dev = selected();
    return dev ? dev->remainingMin : 0;
}

QString MoonrakerDeviceService::selectedDeviceErrorText() const
{
    const Device *dev = selected();
    return dev ? LanErrorCodes::text(dev->errorCode) : QString();
}


// ── 收尾#4：设备文件管理 ──────────────────────────────────────────────────

void MoonrakerDeviceService::requestDeviceFiles()
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        // Moonraker：GET /server/files/list?root=gcodes
        QNetworkRequest req{
            QUrl(moonrakerBase(*dev) + QStringLiteral("/server/files/list?root=gcodes"))};
        req.setTransferTimeout(3000);
        QNetworkReply *reply = manager_->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError)
                return;
            QVariantList files;
            const QJsonArray items = QJsonDocument::fromJson(reply->readAll())
                                         .object()
                                         .value(QStringLiteral("result"))
                                         .toObject()
                                         .value(QStringLiteral("files"))
                                         .toArray();
            for (const QJsonValue &v : items) {
                const QJsonObject f = v.toObject();
                QVariantMap row;
                row.insert(QStringLiteral("name"), f.value(QStringLiteral("name")).toString());
                row.insert(QStringLiteral("size"), f.value(QStringLiteral("size")).toVariant());
                row.insert(QStringLiteral("path"), f.value(QStringLiteral("path")).toString());
                files.append(row);
            }
            emit deviceFilesReady(files);
        });
        return;
    }
    // 4408：ws get pFileList（结果在后续推送里带 pFileList 键 → onWsTextMessage）
    if (ws_ && ws_->state() == QAbstractSocket::ConnectedState) {
        QJsonObject msg;
        msg.insert(QStringLiteral("method"), QStringLiteral("get"));
        QJsonObject params;
        params.insert(QStringLiteral("pFileList"), 1);
        msg.insert(QStringLiteral("params"), params);
        ws_->sendTextMessage(QString::fromUtf8(
            QJsonDocument(msg).toJson(QJsonDocument::Compact)));
    }
}

void MoonrakerDeviceService::printDeviceFile(const QString &fileName)
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        QNetworkRequest req{
            QUrl(moonrakerBase(*dev) + QStringLiteral("/printer/print/start?filename=")
                 + QString::fromUtf8(QUrl::toPercentEncoding(fileName)))};
        manager_->post(req, QByteArray());
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("opGcodeFile"),
                  QStringLiteral("printprt:/") + fileName);
    if (enableSelfTest_)
        params.insert(QStringLiteral("enableSelfTest"), 1);
    wsSet(params);
}

void MoonrakerDeviceService::deleteDeviceFile(const QString &fileName)
{
    Device *dev = selected();
    if (!dev)
        return;
    if (dev->isMoonraker) {
        QNetworkRequest req{
            QUrl(moonrakerBase(*dev) + QStringLiteral("/server/files/delete?path=gcodes/")
                 + QString::fromUtf8(QUrl::toPercentEncoding(fileName)))};
        manager_->deleteResource(req);
        return;
    }
    QJsonObject params;
    params.insert(QStringLiteral("delFile"), fileName);
    wsSet(params);
}


// ── 收尾#5：详细控制 ─────────────────────────────────────────────────────

void MoonrakerDeviceService::setPrintSpeedMode(int mode)
{
    // 官方 speedMode：0=极速 1=标准 2=平稳 3=静音（4408 ws 语义）。
    if (mode < 0 || mode > 3)
        return;
    QJsonObject params;
    params.insert(QStringLiteral("speedMode"), mode);
    wsSet(params);
}

void MoonrakerDeviceService::setChamberLight(bool on)
{
    QJsonObject params;
    params.insert(QStringLiteral("lightSw"), on ? 1 : 0);
    wsSet(params);
}

void MoonrakerDeviceService::setNozzleTemp(int celsius)
{
    if (celsius < 0 || celsius > 300)
        return; // 合理性守卫（官方文案：温度范围不合理）
    QJsonObject params;
    params.insert(QStringLiteral("setNozzleTemp"), celsius);
    wsSet(params);
}

void MoonrakerDeviceService::markDeviceMoonraker(int filteredIndex, int moonrakerPort)
{
    const int real = filteredToReal(filteredIndex);
    if (real == -1)
        return;
    devices_[real].isMoonraker = true;
    devices_[real].moonrakerPort = moonrakerPort;
}
