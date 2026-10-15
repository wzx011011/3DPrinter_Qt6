// X1（审计④章「按角色提取服务接口缝」）接口级单测：注入缝。
//
// 锁定的契约面：
//   - DeviceServiceMock / CloudServiceMock / CameraServiceMock 实现各自的
//     数据面窄接口（IDeviceDataPlane / ICloudDataPlane / ICameraStreamService），
//     消费可全程面向接口（含信号连接）。
//   - 测试可用手写替身（Fake）实现接口注入——替代锁生产 mock 假时序的旧
//     写法（int03 相机状态机的旧断言已按 X1 事件驱动语义改写）。
//   - demo 披露契约：种子假设备条目（编造的 192.168.1.x）绝不被真发送链路
//     当作 FTP/MQTT 目标 host；真实（手动添加）设备在无 MQTT 时绝不假翻
//     printing（U11 失败态契约延续）。
//   - MQTT 上游式 TLS：非 1883 端口一律 ssl://（上游 PrintJob.hpp:77
//     m_local_use_ssl{true} 语义），1883 为显式明文遗留例外。
//   - 相机流事件驱动：不可达 RTSP 收敛 Error 终态、永不假翻 Streaming；
//     stopStream 协作取消及时返回（不再 terminate）。
//
// 不连真机/真 broker/真 RTSP 源；不可达地址用 TEST-NET（192.0.2.0/24，
// RFC 5737 保证不可路由），失败路径时间上界由 FFmpeg stimeout=5s 决定。
#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QVariantMap>

#include "core/services/CameraServiceMock.h"
#include "core/services/CameraStream.h"
#include "core/services/CloudServiceMock.h"
#include "core/services/DeviceServiceMock.h"
#include "core/services/MqttClient.h"

// ── 编译期钉死接口实现关系（与 BackendContext.cpp 装配点断言呼应） ──
static_assert(std::is_base_of_v<IDeviceDataPlane, DeviceServiceMock>,
              "DeviceServiceMock must implement IDeviceDataPlane (X1 seam)");
static_assert(std::is_base_of_v<ICloudDataPlane, CloudServiceMock>,
              "CloudServiceMock must implement ICloudDataPlane (X1 seam)");
static_assert(std::is_base_of_v<ICameraStreamService, CameraServiceMock>,
              "CameraServiceMock must implement ICameraStreamService (X1 seam)");

namespace {

/// X1 注入缝替身：不经任何生产 mock 类型，直接实现窄接口。
/// 证明测试/消费方可以面向接口注入行为（发送契约：started → progress →
/// succeeded 的信号序列由替身自行驱动）。
class FakeDeviceDataPlane final : public IDeviceDataPlane
{
public:
    int progressToSend = 50;

    int deviceCount() const override { return 1; }
    QVariantMap deviceAt(int filteredIndex) const override
    {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("FakePrinter"));
        m.insert(QStringLiteral("demo"), false);
        Q_UNUSED(filteredIndex);
        return m;
    }
    void selectDevice(int filteredIndex) override { Q_UNUSED(filteredIndex); }
    bool addManualDevice(const QString &, const QString &, const QString &, int) override { return true; }
    bool isDemoDevice(int filteredIndex) const override { Q_UNUSED(filteredIndex); return false; }

    QString selectedDeviceAccessCode() const override { return QStringLiteral("FAKE-CODE"); }
    int selectedDeviceMqttPort() const override { return 8883; }
    void setSelectedDeviceAccessCode(const QString &, int) override {}

    bool sendPrintViaFtp(int filteredIndex, const QString &gcodePath) override
    {
        Q_UNUSED(filteredIndex);
        Q_UNUSED(gcodePath);
        emit sendPrintUploadStarted();
        emit sendPrintUploadProgress(progressToSend);
        emit sendPrintSucceeded();
        return true;
    }
    void cancelSendPrint() override { cancelledRequested = true; }
    bool sendPrintInFlight() const override { return false; }

    bool selectedDeviceOnline() const override { return true; }
    QString selectedDeviceStatus() const override { return QStringLiteral("idle"); }
    QString selectedDeviceIp() const override { return QStringLiteral("10.1.2.3"); }
    bool isMqttConnected() const override { return true; }
    int selectedDeviceTemperature() const override { return 0; }
    int selectedDeviceBedTemperature() const override { return 0; }
    int selectedDeviceNozzleTargetTemp() const override { return 0; }
    int selectedDeviceBedTargetTemp() const override { return 0; }
    int selectedDeviceTotalLayerNum() const override { return 0; }
    int selectedDeviceCurrentLayerNum() const override { return 0; }
    int selectedDeviceRemainingTime() const override { return 0; }
    bool applyMqttReportPayload(const QString &, int) override { return false; }

    bool cancelledRequested = false;
};

} // namespace

class ServiceSeamTests : public QObject
{
    Q_OBJECT

private slots:
    // ── 注入缝：mock 实现接口 + 消费面向接口 ──
    void deviceMockServesThroughDataPlaneInterface();
    void cloudMockServesThroughDataPlaneInterface();
    void fakeInjectsThroughInterfaceAndDrivesSendContract();

    // ── X1 真发送链路 provenance 契约 ──
    void demoEntriesNeverBecomeRealSendTargets();
    void realDeviceWithoutMqttNeverFakesPrinting();

    // ── MQTT TLS（上游式 8883） ──
    void mqttBrokerUriTlsByDefault();

    // ── 相机流事件驱动 + 协作取消 ──
    void cameraEventDrivenUnreachableRtspGoesError();
    void cameraNonRtspSourceIsExplicitErrorNotFakeStreaming();
    void cameraStreamStopIsCooperative();
};

void ServiceSeamTests::deviceMockServesThroughDataPlaneInterface()
{
    DeviceServiceMock device;
    IDeviceDataPlane *plane = &device;  // 面向接口消费

    // 设备清单
    QVERIFY(plane->deviceCount() > 0);
    const QVariantMap dev = plane->deviceAt(0);
    QVERIFY2(dev.contains(QStringLiteral("demo")),
             "deviceAt must carry the X1 demo provenance key");
    QCOMPARE(dev.value(QStringLiteral("demo")).toBool(), true);  // 种子条目 = 演示
    QVERIFY(plane->isDemoDevice(0));

    // 凭据面
    QVERIFY(plane->selectedDeviceAccessCode().isEmpty());
    plane->selectDevice(0);
    plane->setSelectedDeviceAccessCode(QStringLiteral("CODE1"), 8883);
    QCOMPARE(plane->selectedDeviceAccessCode(), QStringLiteral("CODE1"));
    QCOMPARE(plane->selectedDeviceMqttPort(), 8883);

    // 遥测面（Bambu report 信封 → 字段）
    const QString report = QStringLiteral(
        "{\"print\":{\"gcode_state\":\"running\",\"mc_percent\":42,"
        "\"bed_temper\":55,\"total_layer_num\":120,\"layer_num\":7,"
        "\"mc_remaining_time\":90}}");
    QVERIFY(plane->applyMqttReportPayload(report, 0));
    QCOMPARE(plane->selectedDeviceStatus(), QStringLiteral("printing"));
    QCOMPARE(plane->selectedDeviceBedTemperature(), 55);
    QCOMPARE(plane->selectedDeviceTotalLayerNum(), 120);
    QCOMPARE(plane->selectedDeviceCurrentLayerNum(), 7);
    QCOMPARE(plane->selectedDeviceRemainingTime(), 90);

    // 手动添加 = 真实条目（demo=false）
    QVERIFY(plane->addManualDevice(QStringLiteral("MyPrinter"),
                                   QStringLiteral("10.0.0.9"),
                                   QStringLiteral("CODE2")));
    const int manualIdx = plane->deviceCount() - 1;
    QVERIFY2(!plane->isDemoDevice(manualIdx),
             "manually added device is a real target, not a demo entry");
    QCOMPARE(plane->deviceAt(manualIdx).value(QStringLiteral("demo")).toBool(), false);
}

void ServiceSeamTests::cloudMockServesThroughDataPlaneInterface()
{
    CloudServiceMock cloud;
    ICloudDataPlane *plane = &cloud;  // 面向接口消费

    QVERIFY(!plane->loggedIn());
    QSignalSpy failedSpy(plane, &ICloudDataPlane::loginFailed);
    QVERIFY(failedSpy.isValid());
    plane->login(QString(), QString());  // 空凭据 → 同步 loginFailed
    QCOMPARE(failedSpy.count(), 1);

    plane->login(QStringLiteral("alice"), QStringLiteral("pw"));
    QTRY_COMPARE_WITH_TIMEOUT(plane->loggedIn(), true, 5000);
    QCOMPARE(plane->userName(), QStringLiteral("alice"));
    QVERIFY(plane->boundDeviceCount() > 0);
    const QVariantMap bound = plane->boundDeviceAt(0);
    QVERIFY(bound.contains(QStringLiteral("sn")));
    QVERIFY(bound.contains(QStringLiteral("online")));

    plane->syncPresets();
    QCOMPARE(plane->syncing(), true);
    QTRY_COMPARE_WITH_TIMEOUT(plane->syncing(), false, 5000);

    plane->logout();
    QVERIFY(!plane->loggedIn());
    QCOMPARE(plane->boundDeviceCount(), 0);
}

void ServiceSeamTests::fakeInjectsThroughInterfaceAndDrivesSendContract()
{
    FakeDeviceDataPlane fake;
    IDeviceDataPlane *plane = &fake;

    // 信号经接口基类 PMF 连接（非虚、moc 在基类生成发射点）——注入缝的
    // 消费面与生产 mock 完全一致。
    QSignalSpy startedSpy(plane, &IDeviceDataPlane::sendPrintUploadStarted);
    QSignalSpy progressSpy(plane, &IDeviceDataPlane::sendPrintUploadProgress);
    QSignalSpy succeededSpy(plane, &IDeviceDataPlane::sendPrintSucceeded);
    QVERIFY(startedSpy.isValid() && progressSpy.isValid() && succeededSpy.isValid());

    QVERIFY(plane->sendPrintViaFtp(0, QStringLiteral("fake:/job.gcode")));
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(progressSpy.at(0).at(0).toInt(), 50);
    QCOMPARE(succeededSpy.count(), 1);

    plane->cancelSendPrint();
    QVERIFY(static_cast<FakeDeviceDataPlane *>(plane)->cancelledRequested);
}

void ServiceSeamTests::demoEntriesNeverBecomeRealSendTargets()
{
    DeviceServiceMock device;
    QSignalSpy failedSpy(&device, &DeviceServiceMock::sendPrintFailed);
    QSignalSpy succeededSpy(&device, &DeviceServiceMock::sendPrintSucceeded);
    QSignalSpy devicesSpy(&device, &DeviceServiceMock::devicesChanged);
    QVERIFY(failedSpy.isValid() && succeededSpy.isValid() && devicesSpy.isValid());

    device.selectDevice(0);
    QVERIFY(device.isDemoDevice(0));  // 种子条目（192.168.1.100 等）
    const QString statusBefore = device.selectedDeviceStatus();
    const int devicesEventsBefore = devicesSpy.count();

    // demo 条目在一切链路检查之前拒绝——绝不成为 FTP/MQTT 目标 host
    QVERIFY(!device.sendPrintViaFtp(0, QStringLiteral("/tmp/owzx_x1_demo.gcode")));
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY2(failedSpy.at(0).at(0).toString().contains(QStringLiteral("演示")),
             "refusal reason must disclose the demo provenance");
    QCOMPARE(succeededSpy.count(), 0);
    QCOMPARE(devicesSpy.count() - devicesEventsBefore, 0);
    QCOMPARE(device.selectedDeviceStatus(), statusBefore);
    QVERIFY(!device.sendPrintInFlight());

    // 二次尝试同样拒绝且不残留半开状态
    QVERIFY(!device.sendPrintViaFtp(0, QString()));
    QCOMPARE(failedSpy.count(), 2);
    QCOMPARE(device.selectedDeviceStatus(), statusBefore);
}

void ServiceSeamTests::realDeviceWithoutMqttNeverFakesPrinting()
{
    DeviceServiceMock device;
    QSignalSpy failedSpy(&device, &DeviceServiceMock::sendPrintFailed);
    QVERIFY(failedSpy.isValid());

    // 手动添加真实打印机（真实 IP + access code），扫描后转 idle/online
    QVERIFY(device.addManualDevice(QStringLiteral("RealPrinter"),
                                   QStringLiteral("10.0.0.20"),
                                   QStringLiteral("CODE")));
    const int idx = device.deviceCount() - 1;
    QVERIFY(!device.isDemoDevice(idx));
    device.scanDevices();
    device.selectDevice(idx);
    QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("idle"));

    // 设备状态事件基线在 setup 之后取样（addManualDevice/scanDevices 本身
    // 合法地发 devicesChanged；下面的断言只看 startPrint 是否翻状态）
    QSignalSpy devicesSpy(&device, &DeviceServiceMock::devicesChanged);
    QVERIFY(devicesSpy.isValid());

    // 真实设备 + MQTT 未连接：绝不动 mock 打印兜底——发可读失败，状态不动
    device.startPrint(idx, QStringLiteral("/tmp/owzx_x1_real.gcode"));
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!failedSpy.at(0).at(0).toString().isEmpty());
    QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("idle"));
    QCOMPARE(devicesSpy.count(), 0);

    // 对照组：demo 设备的显式演示兜底保持（U11 演示语义不回退）
    device.selectDevice(0);
    device.startPrint(0, QString());
    QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("printing"));
}

void ServiceSeamTests::mqttBrokerUriTlsByDefault()
{
    using owzx::MqttClient;
    // 上游 LAN 发送链默认 SSL（PrintJob.hpp:77 m_local_use_ssl{true}）——
    // 8883（及一切非 1883 端口）必须走 ssl://
    QCOMPARE(MqttClient::buildBrokerUri(QStringLiteral("192.168.1.3"), 8883),
             QStringLiteral("ssl://192.168.1.3:8883"));
    QCOMPARE(MqttClient::buildBrokerUri(QStringLiteral("printer.lan"), 990),
             QStringLiteral("ssl://printer.lan:990"));
    // 1883 = 显式明文遗留例外
    QCOMPARE(MqttClient::buildBrokerUri(QStringLiteral("192.168.1.3"), 1883),
             QStringLiteral("tcp://192.168.1.3:1883"));
}

void ServiceSeamTests::cameraEventDrivenUnreachableRtspGoesError()
{
    CameraServiceMock camera;
    ICameraStreamService *cs = &camera;  // 面向接口消费

    QList<int> seq;
    connect(&camera, &CameraServiceMock::streamStatusChanged, &camera,
            [&camera, &seq]() { seq.append(camera.streamStatus()); });

    QCOMPARE(cs->streamStatus(), 0);  // Disconnected
    cs->updateForDevice(QStringLiteral("192.0.2.77"), true);  // TEST-NET 不可达
    QVERIFY(cs->cameraAvailable());

    // 启动即 Connecting（唯一同步发射）；终态由解码事件决定
    cs->startStream();
    QCOMPARE(seq, QList<int>{1});

    // 不可达 RTSP → Error 终态（stimeout 5s，QTRY 裕度覆盖 CI 负载）
    QTRY_COMPARE_WITH_TIMEOUT(cs->streamStatus(), 4, 12000);
    QVERIFY2(!seq.contains(3), "never fake Streaming(3) without a real stream");
    QCOMPARE(seq.first(), 1);
    QCOMPARE(seq.last(), 4);
    QVERIFY(!cs->errorMessage().isEmpty());

    // stopStream 协作取消：及时回 Disconnected
    cs->stopStream();
    QCOMPARE(cs->streamStatus(), 0);
}

void ServiceSeamTests::cameraNonRtspSourceIsExplicitErrorNotFakeStreaming()
{
    CameraServiceMock camera;
    ICameraStreamService *cs = &camera;

    cs->updateForDevice(QStringLiteral("192.0.2.78"), true);
    camera.setCameraUrl(QStringLiteral("http://not-rtsp.example/stream"));
    cs->startStream();
    // 非 RTSP 源（真实相机后端未接入的形态）→ 显式错误披露，绝不"直播中"
    QCOMPARE(cs->streamStatus(), 4);
    QVERIFY(!cs->errorMessage().isEmpty());
    cs->stopStream();
    QCOMPARE(cs->streamStatus(), 0);
}

void ServiceSeamTests::cameraStreamStopIsCooperative()
{
    owzx::CameraStream stream;
    stream.setUrl(QStringLiteral("rtsp://192.0.2.9:8554/live"));
    QElapsedTimer timer;
    timer.start();
    stream.startStream();
    stream.stopStream();
    // 协作取消（interrupt callback 中断 avformat_open_input 阻塞）应在
    // wait(3000) 上限内汇合，绝不 terminate 强杀
    QVERIFY2(timer.elapsed() < 8000,
             qPrintable(QStringLiteral("stopStream took %1 ms; cooperative "
                                       "cancel should abort blocking IO")
                            .arg(timer.elapsed())));
    QVERIFY(!stream.isRunning());
}

QTEST_MAIN(ServiceSeamTests)
#include "ServiceSeamTests.moc"
