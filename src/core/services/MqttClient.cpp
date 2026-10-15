#include "MqttClient.h"

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QUuid>
#include <cstring>

// paho-mqtt C 异步接口（DEPS_PREFIX/include）
namespace owzx {

QString MqttClient::buildBrokerUri(const QString &host, int port)
{
    // X1: 上游式 TLS——Bambu/Creality LAN 发送链默认 SSL（上游
    // PrintJob.hpp:77-78 m_local_use_ssl{true} / m_local_use_ssl_for_ftp{true}）。
    // 此前这里无条件 tcp:// 明文，即使上层传 8883 也不带 SSL 选项——访问码
    // 明文过局域网。端口 1883 保留为显式明文遗留例外（协议注释兼容位）。
    const bool useTls = (port != 1883);
    return QStringLiteral("%1://%2:%3")
        .arg(useTls ? QStringLiteral("ssl") : QStringLiteral("tcp"), host)
        .arg(port);
}

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
    , m_clientId(QString("owzx-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8)))
{
}

MqttClient::~MqttClient()
{
    disconnectFromHost();
}

int MqttClient::reconnectDelayMs(int attempts)
{
    // 指数退避：1s 起步倍增，封顶 60s（1s<<6=64s 已超上限，min 兜住）
    const qint64 delay = 1000LL << qBound(0, attempts, 6);
    return static_cast<int>(qMin(delay, static_cast<qint64>(kMaxReconnectDelayMs)));
}

void MqttClient::resetClient()
{
    if (m_client)
    {
        MQTTAsync_destroy(&m_client);
        m_client = nullptr;
    }
    m_subscribedTopics.clear();
}

#if OWZX_HAS_PAHO_MQTT

void MqttClient::connectToHost(const QString &host, int port, const QString &accessCode)
{
    // X1: 显式连接入口 —— 停掉退避重连，清自动重连标记
    if (m_reconnectTimer)
        m_reconnectTimer->stop();
    m_reconnectPending = false;
    m_userDisconnected = false;
    m_reconnectAttempts = 0;

    m_host = host;
    m_port = port;
    m_tls = (port != 1883);

    // 如果有访问码，设为 username/password（Bambu 协议: username="bblp", password=accessCode）
    // v2.8 review C2: credentials stored as members (was static → race on reconnect)
    m_password = accessCode.isEmpty() ? QByteArray() : accessCode.toUtf8();

    if (m_client)
        resetClient();
    openConnection();
}

void MqttClient::openConnection()
{
    // 如果已连接或正在连接，先断开
    if (m_state == Connected || m_state == Connecting)
        disconnectFromHost();

    const QString uri = buildBrokerUri(m_host, m_port);

    qInfo("[MQTT] connecting to %s (clientId=%s, tls=%d)",
          uri.toUtf8().constData(), m_clientId.toUtf8().constData(), m_tls ? 1 : 0);

    MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
    opts.keepAliveInterval = 60;
    opts.cleansession = 1;
    opts.onSuccess = &MqttClient::onConnect;
    opts.onFailure = &MqttClient::onConnectFailure;
    opts.context = this;
    opts.connectTimeout = 5;  // 5 秒超时
    // X1: paho 内建自动重连关闭——重连策略由 connectionLost 驱动的指数退避
    // 统一管理（对上层有明确的 connectionLost 信号与可观察的 attempts）。
    opts.automaticReconnect = 0;

    if (!m_password.isEmpty())
    {
        opts.username = m_username.constData();
        opts.password = m_password.constData();
    }

    if (m_tls)
    {
        // X1: 证书校验策略对照上游 Bambu 协议——打印机证书为自签名（无公开
        // CA 链），上游 LAN 链路以自签名证书直连（本仓 FTPS 侧 FtpUploader
        // 的 CURLOPT_SSL_VERIFYPEER=0 同一策略）。TLS 通道本身加密访问码，
        // 消除明文 1883 的嗅探面。
        m_sslOptions = MQTTAsync_SSLOptions_initializer;
        m_sslOptions.enableServerCertAuth = 0;
        m_sslOptions.verify = 0;
        opts.ssl = &m_sslOptions;
    }

    // 创建 MQTT 客户端
    int rc = MQTTAsync_create(&m_client,
                              uri.toUtf8().constData(),
                              m_clientId.toUtf8().constData(),
                              MQTTCLIENT_PERSISTENCE_NONE,
                              nullptr);
    if (rc != MQTTASYNC_SUCCESS)
    {
        qWarning("[MQTT] create failed: %d", rc);
        m_client = nullptr;
        handleConnectFailure(rc);
        return;
    }

    // 设置回调（参数顺序: client, context, connectionLost, messageArrived,
    // deliveryComplete）。X1: connectionLost 此前传 nullptr——掉线无感知。
    MQTTAsync_setCallbacks(m_client, this, &MqttClient::onConnectionLost,
                           &MqttClient::onMessageArrived, nullptr);

    // 发起连接
    setState(Connecting);
    rc = MQTTAsync_connect(m_client, &opts);
    if (rc != MQTTASYNC_SUCCESS)
    {
        qWarning("[MQTT] connect failed (sync): %d", rc);
        // 同步失败路径：句柄未注册任何回调，直接收尾后走统一失败汇聚点
        resetClient();
        handleConnectFailure(rc);
    }
}

void MqttClient::handleConnectFailure(int code)
{
    setState(ConnectionFailed);
    emit errorOccurred(QString("MQTT connect failed: %1").arg(code));

    if (m_reconnectPending && !m_userDisconnected)
    {
        // 自动重连在途中的失败 → 续排下一轮退避
        scheduleReconnect();
    }
    else if (!m_userDisconnected && !m_client)
    {
        // 初连失败：保持 ConnectionFailed 终态，交上层（MonitorViewModel
        // 超时回退 / 用户重试入口）驱动下一次尝试，不做后台无限重试。
        qWarning("[MQTT] initial connect failed (%d); no auto-retry for explicit connect", code);
    }
}

void MqttClient::disconnectFromHost()
{
    m_userDisconnected = true;
    if (m_reconnectTimer)
        m_reconnectTimer->stop();
    m_reconnectPending = false;

    if (!m_client)
    {
        // 掉线后（连接丢失，state 已是 Disconnected 但句柄仍在）的显式断开：
        // 照样收尾，避免下一次 connectToHost 覆盖句柄造成泄漏。
        m_subscribedTopics.clear();
        setState(Disconnected);
        return;
    }

    qInfo("[MQTT] disconnecting");
    if (m_state == Connected || m_state == Connecting)
    {
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.context = this;
        MQTTAsync_disconnect(m_client, &opts);
    }

    resetClient();
    setState(Disconnected);
}

bool MqttClient::subscribe(const QString &topic)
{
    if (!isConnected() || !m_client)
    {
        qWarning("[MQTT] subscribe failed: not connected");
        return false;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = &MqttClient::onSubscribe;
    opts.onFailure = &MqttClient::onSubscribeFailure;
    opts.context = this;

    int rc = MQTTAsync_subscribe(m_client, topic.toUtf8().constData(), 0, &opts);
    if (rc != MQTTASYNC_SUCCESS)
    {
        qWarning("[MQTT] subscribe to %s failed: %d", topic.toUtf8().constData(), rc);
        return false;
    }

    if (!m_subscribedTopics.contains(topic))
        m_subscribedTopics.append(topic);
    qDebug("[MQTT] subscribed: %s", topic.toUtf8().constData());
    return true;
}

bool MqttClient::unsubscribe(const QString &topic)
{
    if (!isConnected() || !m_client)
        return false;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = this;
    int rc = MQTTAsync_unsubscribe(m_client, topic.toUtf8().constData(), &opts);
    if (rc == MQTTASYNC_SUCCESS)
        m_subscribedTopics.removeAll(topic);
    return rc == MQTTASYNC_SUCCESS;
}

bool MqttClient::publish(const QString &topic, const QString &payload)
{
    if (!isConnected() || !m_client)
    {
        qWarning("[MQTT] publish failed: not connected");
        return false;
    }

    QByteArray data = payload.toUtf8();
    MQTTAsync_message msg = MQTTAsync_message_initializer;
    msg.payload = data.data();
    msg.payloadlen = data.size();
    msg.qos = 0;
    msg.retained = 0;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = this;

    MQTTAsync_token token;
    int rc = MQTTAsync_sendMessage(m_client, topic.toUtf8().constData(), &msg, &opts);
    if (rc != MQTTASYNC_SUCCESS)
    {
        qWarning("[MQTT] publish to %s failed: %d", topic.toUtf8().constData(), rc);
        return false;
    }
    qDebug("[MQTT] published %d bytes to %s", data.size(), topic.toUtf8().constData());
    return true;
}

#else // !OWZX_HAS_PAHO_MQTT

void MqttClient::connectToHost(const QString &host, int port, const QString &accessCode)
{
    Q_UNUSED(accessCode);
    m_host = host;
    m_port = port;
    m_tls = (port != 1883);
    if (m_reconnectTimer)
        m_reconnectTimer->stop();
    m_reconnectPending = false;
    m_userDisconnected = false;
    qWarning("[MQTT] paho-mqtt is not available; real MQTT connection disabled");
    setState(ConnectionFailed);
    emit errorOccurred(QStringLiteral("MQTT support is not available in this build"));
}

void MqttClient::disconnectFromHost()
{
    m_userDisconnected = true;
    m_subscribedTopics.clear();
    m_client = nullptr;
    setState(Disconnected);
}

bool MqttClient::subscribe(const QString &topic)
{
    Q_UNUSED(topic);
    return false;
}

bool MqttClient::unsubscribe(const QString &topic)
{
    Q_UNUSED(topic);
    return false;
}

bool MqttClient::publish(const QString &topic, const QString &payload)
{
    Q_UNUSED(topic);
    Q_UNUSED(payload);
    return false;
}

#endif

void MqttClient::setState(ConnectionState state)
{
    if (m_state != state)
    {
        m_state = state;
        emit stateChanged(state);
    }
}

// ── X1: 自动重连（本对象线程） ──────────────────────────────────

void MqttClient::scheduleReconnect()
{
    if (!m_reconnectTimer)
    {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, &MqttClient::attemptReconnect);
    }
    const int delay = reconnectDelayMs(m_reconnectAttempts++);
    qInfo("[MQTT] auto-reconnect scheduled in %d ms (attempt %d)", delay, m_reconnectAttempts);
    m_reconnectTimer->start(delay);
}

void MqttClient::attemptReconnect()
{
    if (m_userDisconnected || m_state == Connected || m_state == Connecting)
        return;
    qInfo("[MQTT] auto-reconnect attempt %d to %s:%d",
          m_reconnectAttempts, m_host.toUtf8().constData(), m_port);
    if (m_client)
        resetClient();
    openConnection();
}

void MqttClient::handleConnected()
{
    m_reconnectPending = false;
    m_reconnectAttempts = 0;
    setState(Connected);
}

void MqttClient::handleConnectionLost(const QString &cause)
{
    if (m_userDisconnected)
        return;  // 显式断开期间 paho 的迟到回调
    qWarning("[MQTT] connection lost: %s",
             cause.isEmpty() ? QStringLiteral("(no cause)").toUtf8().constData()
                             : cause.toUtf8().constData());
    resetClient();
    setState(Disconnected);
    emit connectionLost(cause);

    // 指数退避自动重连（1s → 2s → 4s → ... 封顶 60s，连接成功归零）
    m_reconnectPending = true;
    scheduleReconnect();
}

// ── paho-mqtt C 回调（static，转发到 Qt 信号）──

#if OWZX_HAS_PAHO_MQTT

void MqttClient::onConnect(void *context, MQTTAsync_successData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    qDebug("[MQTT] connected");
    // 计数器归零/标记清理只在本对象线程做（与 scheduleReconnect 同线程，
    // 避免 paho 线程直改状态）
    QMetaObject::invokeMethod(self, &MqttClient::handleConnected, Qt::QueuedConnection);
    Q_UNUSED(response);
}

void MqttClient::onConnectFailure(void *context, MQTTAsync_failureData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    const int code = response ? response->code : -1;
    qWarning("[MQTT] connect failure: %d", code);
    // 汇聚点在本对象线程执行（paho 回调线程 → 队列）
    QMetaObject::invokeMethod(self, [self, code]() { self->handleConnectFailure(code); },
                              Qt::QueuedConnection);
}

void MqttClient::onConnectionLost(void *context, char *cause)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    // X1: 此前该回调位传 nullptr。回调在 paho 内部线程执行，跨线程队列到
    // 本对象线程处理（与 v2.8 review C3 的 messageReceived 同一纪律）。
    const QString reason = cause ? QString::fromUtf8(cause) : QString();
    QMetaObject::invokeMethod(self, [self, reason]() { self->handleConnectionLost(reason); },
                              Qt::QueuedConnection);
}

void MqttClient::onDisconnect(void *context, MQTTAsync_successData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    qDebug("[MQTT] disconnected");
    self->setState(Disconnected);
}

int MqttClient::onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    // v2.8 review C3: 此回调在 paho 内部线程执行。emit 跨线程到 UI 线程的 slot
    // 时，Qt AutoConnection 自动选择 QueuedConnection（self 的 thread affinity 是 UI
    // 线程，当前线程是 paho 线程 → 自动队列化）。因此 messageReceived 的 slot 在
    // UI 线程执行，写 devices_/printJobs_ 无竞态。确认安全。
    auto *self = static_cast<MqttClient *>(context);
    if (!self || !message)
        return 1;

    auto *msg = message;

    // 提取 topic
    int len = topicLen > 0 ? topicLen : static_cast<int>(strlen(topicName));
    QString topic = QString::fromUtf8(topicName, len);

    // 提取 payload（JSON）
    QString payload = QString::fromUtf8(static_cast<const char *>(msg->payload), msg->payloadlen);

    qDebug("[MQTT] message on %s: %d bytes", topic.toUtf8().constData(), msg->payloadlen);
    emit self->messageReceived(topic, payload);

    MQTTAsync_freeMessage(&msg);
    MQTTAsync_free(topicName);
    return 1;
}

void MqttClient::onSubscribe(void *context, MQTTAsync_successData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (self) qDebug("[MQTT] subscribe success");
}

void MqttClient::onSubscribeFailure(void *context, MQTTAsync_failureData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    int code = response ? response->code : -1;
    qWarning("[MQTT] subscribe failure: %d", code);
}

#else

void MqttClient::onConnect(void *context, MQTTAsync_successData *response)
{
    Q_UNUSED(context);
    Q_UNUSED(response);
}

void MqttClient::onConnectFailure(void *context, MQTTAsync_failureData *response)
{
    Q_UNUSED(context);
    Q_UNUSED(response);
}

void MqttClient::onDisconnect(void *context, MQTTAsync_successData *response)
{
    Q_UNUSED(context);
    Q_UNUSED(response);
}

int MqttClient::onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    Q_UNUSED(context);
    Q_UNUSED(topicName);
    Q_UNUSED(topicLen);
    Q_UNUSED(message);
    return 1;
}

void MqttClient::onSubscribe(void *context, MQTTAsync_successData *response)
{
    Q_UNUSED(context);
    Q_UNUSED(response);
}

void MqttClient::onSubscribeFailure(void *context, MQTTAsync_failureData *response)
{
    Q_UNUSED(context);
    Q_UNUSED(response);
}

#endif

} // namespace owzx
