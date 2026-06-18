#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <functional>

// v2.5 DEV-01: MQTT 客户端封装（基于 paho-mqtt C 异步接口）
// 对齐上游 bambu_networking 的 MQTT 通信层
//
// 用途: 连接 3D 打印机 MQTT broker, 订阅设备状态, 发布控制命令
// 协议: Bambu/Creality 打印机用 MQTT (端口 8883 TLS / 1883 明文)
//
// 异步模式: paho-mqtt MQTTAsync API, 回调通过 Qt 信号通知 UI 层

// 直接 include paho-mqtt 头（避免前置声明冲突）
#include <MQTTAsync.h>

namespace owzx {

class MqttClient : public QObject
{
    Q_OBJECT
public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient();

    /// 连接状态
    enum ConnectionState {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        ConnectionFailed = 3
    };
    Q_ENUM(ConnectionState)

    // ── 连接管理 ──
    /// 连接到打印机 MQTT broker
    /// host: 打印机 IP, port: 8883(TLS)/1883(明文), accessCode: 访问码(可选)
    Q_INVOKABLE void connectToHost(const QString &host, int port, const QString &accessCode = "");
    Q_INVOKABLE void disconnectFromHost();

    /// 当前连接状态
    Q_INVOKABLE ConnectionState state() const { return m_state; }
    Q_INVOKABLE bool isConnected() const { return m_state == Connected; }

    // ── 订阅/发布 ──
    /// 订阅 topic（对齐 Bambu: device/<serial>/report）
    Q_INVOKABLE bool subscribe(const QString &topic);
    Q_INVOKABLE bool unsubscribe(const QString &topic);

    /// 发布消息（对齐 Bambu: device/<serial>/request）
    Q_INVOKABLE bool publish(const QString &topic, const QString &payload);

    /// 当前已订阅 topic 列表
    QStringList subscribedTopics() const { return m_subscribedTopics; }

signals:
    /// 连接状态变化
    void stateChanged(ConnectionState state);
    /// 收到订阅消息（topic + payload JSON）
    void messageReceived(const QString &topic, const QString &payload);
    /// 连接错误
    void errorOccurred(const QString &error);

private:
    // paho-mqtt C 句柄
    MQTTAsync m_client = nullptr;
    // 客户端 ID（唯一标识）
    QString m_clientId;
    // 连接状态
    ConnectionState m_state = Disconnected;
    // 已订阅 topic
    QStringList m_subscribedTopics;
    // broker 地址
    QString m_host;
    int m_port = 8883;

    // paho-mqtt C 回调（static, 转发到 Qt 信号）
    static void onConnect(void *context, MQTTAsync_successData *response);
    static void onConnectFailure(void *context, MQTTAsync_failureData *response);
    static void onDisconnect(void *context, MQTTAsync_successData *response);
    static int onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
    static void onSubscribe(void *context, MQTTAsync_successData *response);
    static void onSubscribeFailure(void *context, MQTTAsync_failureData *response);

    // 内部状态设置
    void setState(ConnectionState state);
};

} // namespace owzx
