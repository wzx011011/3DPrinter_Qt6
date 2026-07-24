#include "MqttClient.h"

#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include <cstring>

// paho-mqtt C 异步接口（DEPS_PREFIX/include）
namespace owzx {

MqttClient::MqttClient(QObject *parent)
    : QObject(parent)
    , m_clientId(QString("owzx-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8)))
{
}

MqttClient::~MqttClient()
{
    disconnectFromHost();
}

#if OWZX_HAS_PAHO_MQTT

void MqttClient::connectToHost(const QString &host, int port, const QString &accessCode)
{
    // 如果已连接或正在连接，先断开
    if (m_state == Connected || m_state == Connecting)
        disconnectFromHost();

    m_host = host;
    m_port = port;

    // 构建 broker URI（Bambu/Creality 默认 TLS 8883，简化用明文 1883）
    QString uri = QString("tcp://%1:%2").arg(host).arg(port);

    qInfo("[MQTT] connecting to %s (clientId=%s)", uri.toUtf8().constData(), m_clientId.toUtf8().constData());

    MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
    opts.keepAliveInterval = 60;
    opts.cleansession = 1;
    opts.onSuccess = &MqttClient::onConnect;
    opts.onFailure = &MqttClient::onConnectFailure;
    opts.context = this;
    opts.connectTimeout = 5;  // 5 秒超时

    // 如果有访问码，设为 username/password（Bambu 协议: username="bblp", password=accessCode）
    // v2.8 review C2: credentials stored as members (was static → race on reconnect)
    if (!accessCode.isEmpty())
    {
        m_password = accessCode.toUtf8();
        opts.username = m_username.constData();
        opts.password = m_password.constData();
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
        setState(ConnectionFailed);
        emit errorOccurred(QString("MQTT create failed: %1").arg(rc));
        return;
    }

    // 设置消息到达回调（参数顺序: client, context, connectionLost, messageArrived, deliveryComplete）
    MQTTAsync_setCallbacks(m_client, this, nullptr, &MqttClient::onMessageArrived, nullptr);

    // 发起连接
    setState(Connecting);
    rc = MQTTAsync_connect(m_client, &opts);
    if (rc != MQTTASYNC_SUCCESS)
    {
        qWarning("[MQTT] connect failed: %d", rc);
        setState(ConnectionFailed);
        emit errorOccurred(QString("MQTT connect failed: %1").arg(rc));
        MQTTAsync_destroy(&m_client);
        m_client = nullptr;
    }
}

void MqttClient::disconnectFromHost()
{
    if (!m_client || m_state == Disconnected)
        return;

    qInfo("[MQTT] disconnecting");
    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
    opts.context = this;
    MQTTAsync_disconnect(m_client, &opts);

    MQTTAsync_destroy(&m_client);
    m_client = nullptr;
    m_subscribedTopics.clear();
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

#else

void MqttClient::connectToHost(const QString &host, int port, const QString &accessCode)
{
    Q_UNUSED(accessCode);
    m_host = host;
    m_port = port;
    qWarning("[MQTT] paho-mqtt is not available; real MQTT connection disabled");
    setState(ConnectionFailed);
    emit errorOccurred(QStringLiteral("MQTT support is not available in this build"));
}

void MqttClient::disconnectFromHost()
{
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

// ── paho-mqtt C 回调（static，转发到 Qt 信号）──

#if OWZX_HAS_PAHO_MQTT

void MqttClient::onConnect(void *context, MQTTAsync_successData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    qDebug("[MQTT] connected");
    self->setState(Connected);
}

void MqttClient::onConnectFailure(void *context, MQTTAsync_failureData *response)
{
    auto *self = static_cast<MqttClient *>(context);
    if (!self) return;
    int code = response ? response->code : -1;
    qWarning("[MQTT] connect failure: %d", code);
    self->setState(ConnectionFailed);
    emit self->errorOccurred(QString("MQTT connect failed: %1").arg(code));
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
