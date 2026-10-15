#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <functional>

class QTimer;

// v2.5 DEV-01: MQTT 客户端封装（基于 paho-mqtt C 异步接口）
// 对齐上游 bambu_networking 的 MQTT 通信层
//
// 用途: 连接 3D 打印机 MQTT broker, 订阅设备状态, 发布控制命令
// 协议: Bambu/Creality 打印机用 MQTT（X1: 默认 TLS 8883，上游 LAN 发送链
//       m_local_use_ssl{true}（PrintJob.hpp:77-78）；端口 1883 为显式遗留
//       明文例外）
//
// 异步模式: paho-mqtt MQTTAsync API, 回调通过 Qt 信号通知 UI 层
// X1: connectionLost 已接线（此前传 nullptr——掉线无感知），断线后按指数
//     退避自动重连（1s 起步、倍增、封顶 60s，连接成功归零）。

// 直接 include paho-mqtt 头（避免前置声明冲突）
#if defined(__has_include)
#  if __has_include(<MQTTAsync.h>)
#    include <MQTTAsync.h>
#    define OWZX_HAS_PAHO_MQTT 1
#  else
#    define OWZX_HAS_PAHO_MQTT 0
using MQTTAsync = void *;
struct MQTTAsync_successData;
struct MQTTAsync_failureData;
struct MQTTAsync_message;
struct MQTTAsync_SSLOptions;
#  endif
#else
#  include <MQTTAsync.h>
#  define OWZX_HAS_PAHO_MQTT 1
#endif

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
    /// host: 打印机 IP, port: 8883(TLS，默认)/1883(显式明文遗留), accessCode: 访问码(可选)
    Q_INVOKABLE void connectToHost(const QString &host, int port, const QString &accessCode = "");
    Q_INVOKABLE void disconnectFromHost();

    /// 当前连接状态
    Q_INVOKABLE ConnectionState state() const { return m_state; }
    Q_INVOKABLE bool isConnected() const { return m_state == Connected; }

    /// X1: 构建 broker URI——端口 1883 = tcp://（显式明文遗留），其余 = ssl://
    /// （默认 TLS 8883，上游 LAN 链 m_local_use_ssl{true} 语义）。
    /// 独立成纯函数供测试断言（对齐 FtpUploader::buildFtpUrl 先例）。
    static QString buildBrokerUri(const QString &host, int port);

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
    /// X1: 已建立的连接意外断开（paho connectionLost 回调经队列投递到本对象线程）。
    /// 此前回调位传 nullptr，掉线对上层完全无感知。自动重连由内部指数退避驱动，
    /// 本信号供上层感知/提示（重连成功后 stateChanged(Connected) 照常发出）。
    void connectionLost(const QString &reason);

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
    // X1: 本连接是否走 TLS（端口 1883 = 显式明文遗留，其余 TLS）
    bool m_tls = true;
    // v2.8 review C2: credentials stored as members (lifetime tied to MqttClient,
    // safe across async paho callbacks). Was static buffer → race/UAF on reconnect.
    QByteArray m_username = "bblp";
    QByteArray m_password;  ///< access code (set before connect, kept alive until disconnect)
#if OWZX_HAS_PAHO_MQTT
    // X1: TLS 选项必须存活到 paho 异步连接完成（对齐 m_password 生命周期纪律）。
    // 仅 paho 构建存在（无 paho 的回退构建里该类型只有前置声明，不能作成员）。
    MQTTAsync_SSLOptions m_sslOptions{};
#endif

    // ── X1: connectionLost 驱动的指数退避自动重连（全部在本对象线程执行） ──
    QTimer *m_reconnectTimer = nullptr;
    int m_reconnectAttempts = 0;
    bool m_reconnectPending = false;    ///< 掉线后的自动重连在途（初连失败不进入）
    bool m_userDisconnected = false;    ///< 显式断开期间吞掉 paho 迟到的 lost/failure 回调
    static constexpr int kMaxReconnectDelayMs = 60 * 1000;
    /// 指数退避：1s, 2s, 4s, ... 封顶 60s
    static int reconnectDelayMs(int attempts);

    // 内部：connectToHost / 自动重连共用的实际连接路径（本对象线程）
    void openConnection();
    // 内部：销毁 paho 句柄 + 清订阅表（连接结束/重建前的统一收尾）
    void resetClient();
    // 内部：UI 线程回调汇聚点（paho 线程回调经 invokeMethod 队列到此处）
    void handleConnected();
    void handleConnectFailure(int code);
    void handleConnectionLost(const QString &cause);
    void scheduleReconnect();
    void attemptReconnect();

    // paho-mqtt C 回调（static, 转发到 Qt 信号）
    static void onConnect(void *context, MQTTAsync_successData *response);
    static void onConnectFailure(void *context, MQTTAsync_failureData *response);
    static void onDisconnect(void *context, MQTTAsync_successData *response);
    static void onConnectionLost(void *context, char *cause);
    static int onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
    static void onSubscribe(void *context, MQTTAsync_successData *response);
    static void onSubscribeFailure(void *context, MQTTAsync_failureData *response);

    // 内部状态设置
    void setState(ConnectionState state);
};

} // namespace owzx
