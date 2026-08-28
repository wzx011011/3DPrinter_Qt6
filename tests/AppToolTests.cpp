// AppToolTests — AI control surface regression (OWzx-only, docs/ai-control.md).
//
// Exercises the full headless stack: real ProjectServiceMock/SliceService/
// ConfigViewModel/EditorViewModel + AppToolRegistry + McpHttpServer over a
// loopback socket. No GPU, no sidecar, no network beyond 127.0.0.1.
//
// AUTOMOC note (same as ViewModelSmokeTests): the canonical verify script
// re-runs cmake configure after slot edits; for incremental builds delete
// build/AppToolTests_autogen/timestamp.

#include <QtTest>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QSet>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QTimer>
#include <functional>

#include "core/ai/AppToolRegistry.h"
#include "core/ai/McpHttpServer.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"

#include <memory>

namespace {
const QString kStlPath = QDir::cleanPath(
    QStringLiteral(QT_TESTCASE_SOURCEDIR) +
    QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));
const QString kTestToken = QStringLiteral("app-tool-test-token");

struct HttpResult {
  int status = 0;
  QByteArray body;
};

// Minimal event-loop-driven HTTP/1.1 client for loopback tests. The server
// (QTcpServer) only services connections from the event loop, so blocking
// waitFor* calls on the client socket would starve it — hence QEventLoop.
HttpResult httpPost(quint16 port, const QByteArray &method, const char *path,
                    const QByteArray &body, const QByteArray &authHeader,
                    const QByteArray &hostOverride = {}) {
  HttpResult out;
  QTcpSocket socket;
  QByteArray raw;
  QEventLoop loop;
  QTimer::singleShot(4000, &loop, &QEventLoop::quit);
  QObject::connect(&socket, &QTcpSocket::connected, &loop, [&]() {
    QByteArray req;
    req += method + " " + path + " HTTP/1.1\r\n";
    req += "Host: " +
           (hostOverride.isEmpty()
                ? QByteArray("127.0.0.1:") + QByteArray::number(port)
                : hostOverride) +
           "\r\n";
    req += "Content-Type: application/json\r\n";
    req += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    if (!authHeader.isEmpty())
      req += "Authorization: " + authHeader + "\r\n";
    req += "Connection: close\r\n\r\n";
    req += body;
    socket.write(req);
  });
  QObject::connect(&socket, &QTcpSocket::readyRead, &loop,
                   [&]() { raw += socket.readAll(); });
  QObject::connect(&socket, &QTcpSocket::disconnected, &loop,
                   &QEventLoop::quit);
  QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop,
                   &QEventLoop::quit);
  socket.connectToHost(QHostAddress::LocalHost, port);
  loop.exec();
  socket.abort();
  const int statusEnd = raw.indexOf(' ');
  if (statusEnd < 0)
    return out;
  out.status = raw.mid(statusEnd + 1, 3).toInt();
  const int bodyStart = raw.indexOf("\r\n\r\n");
  if (bodyStart >= 0)
    out.body = raw.mid(bodyStart + 4);
  return out;
}

QJsonObject jsonRpcRequest(const QString &method, const QJsonObject &params,
                           int id) {
  QJsonObject r;
  r.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
  r.insert(QStringLiteral("id"), id);
  r.insert(QStringLiteral("method"), method);
  r.insert(QStringLiteral("params"), params);
  return r;
}
}  // namespace

class AppToolTests final : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void cleanupTestCase();

  void toolDefinitionsExposeSchemaForEveryTool();
  void unknownToolAndBadArgsFailCleanly();
  void appStateSceneObjectInfoRoundTrip();
  void configListAndSetValueRoundTrip();
  void transformDuplicateDeleteRoundTrip();
  void sliceGatesRejectInvalidRequests();
  void mcpServerInitializeListAndCall();
  void mcpServerAuthAndErrorRouting();

 private:
  std::unique_ptr<PresetServiceMock> preset_;
  std::unique_ptr<ProjectServiceMock> project_;
  std::unique_ptr<SliceService> slice_;
  std::unique_ptr<ConfigViewModel> config_;
  std::unique_ptr<EditorViewModel> editor_;
  std::unique_ptr<UndoRedoManager> undo_;
  std::unique_ptr<OWzx::AppToolRegistry> registry_;
};

void AppToolTests::initTestCase() {
  QVERIFY2(QFileInfo::exists(kStlPath),
           qPrintable(QStringLiteral("Test STL not found: %1").arg(kStlPath)));

  preset_ = std::make_unique<PresetServiceMock>();
  project_ = std::make_unique<ProjectServiceMock>();
  slice_ = std::make_unique<SliceService>(project_.get());
  config_ = std::make_unique<ConfigViewModel>(preset_.get(), project_.get());
  editor_ = std::make_unique<EditorViewModel>(project_.get(), slice_.get());
  // BackendContext injects this in the app; without it delete/duplicate are
  // not undoable and the undo-tool assertions below would be vacuous.
  undo_ = std::make_unique<UndoRedoManager>();
  editor_->setUndoRedoManager(undo_.get());
  registry_ = std::make_unique<OWzx::AppToolRegistry>(
      project_.get(), slice_.get(), config_.get(), editor_.get());

  // Load one real model so the scene/config tools have live state.
  QSignalSpy loadSpy(project_.get(), &ProjectServiceMock::loadFinished);
  QVERIFY(project_->loadFile(kStlPath));
  QVERIFY2(loadSpy.isValid(), "loadFinished spy must be valid");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY(project_->modelCount() >= 1);
}

void AppToolTests::cleanupTestCase() {
  registry_.reset();
  editor_.reset();
  undo_.reset();
  config_.reset();
  slice_.reset();
  project_.reset();
  preset_.reset();
}

void AppToolTests::toolDefinitionsExposeSchemaForEveryTool() {
  const QJsonArray defs = registry_->toolDefinitions();
  QVERIFY2(defs.size() >= 20, "expected at least 20 tools exposed");
  QSet<QString> names;
  for (const QJsonValue &v : defs) {
    const QJsonObject d = v.toObject();
    const QString name = d.value(QStringLiteral("name")).toString();
    QVERIFY2(!name.isEmpty(), "every tool must have a name");
    QVERIFY2(!names.contains(name), qPrintable(QStringLiteral("duplicate tool name: %1").arg(name)));
    names.insert(name);
    QVERIFY2(d.value(QStringLiteral("description")).toString().size() > 10,
             "every tool must carry a useful description");
    const QJsonObject schema = d.value(QStringLiteral("inputSchema")).toObject();
    QCOMPARE(schema.value(QStringLiteral("type")).toString(), QStringLiteral("object"));
    QVERIFY2(schema.contains(QStringLiteral("properties")),
             "inputSchema must declare properties");

    // GLM's Anthropic-compatible endpoint rejects union type strings such as
    // "string|number" with API error 1210 (claude.exe forwards our schemas
    // verbatim). Every declared property type must be a concrete JSON-Schema
    // simple type, recursively into nested objects.
    static const QSet<QString> kValidTypes = {
        QStringLiteral("object"), QStringLiteral("string"),
        QStringLiteral("number"), QStringLiteral("integer"),
        QStringLiteral("boolean"), QStringLiteral("array")};
    std::function<void(const QJsonObject &)> checkProps = [&](const QJsonObject &props) {
      for (auto it = props.begin(); it != props.end(); ++it) {
        const QJsonObject p = it.value().toObject();
        const QString t = p.value(QStringLiteral("type")).toString();
        QVERIFY2(!t.contains(QLatin1Char('|')),
                 qPrintable(QStringLiteral("union type in %1.%2: %3").arg(name, it.key(), t)));
        QVERIFY2(kValidTypes.contains(t),
                 qPrintable(QStringLiteral("invalid type in %1.%2: %3").arg(name, it.key(), t)));
        if (t == QLatin1String("object"))
          checkProps(p.value(QStringLiteral("properties")).toObject());
      }
    };
    checkProps(schema.value(QStringLiteral("properties")).toObject());
  }
  // Core coverage: visible + invisible control both present.
  for (const char *expected :
       {"get_app_state", "get_scene", "list_config_keys", "set_config_value",
        "slice_plate", "export_gcode", "switch_page", "undo", "clear_project"}) {
    QVERIFY2(registry_->hasTool(QString::fromLatin1(expected)),
             qPrintable(QStringLiteral("missing core tool: %1").arg(expected)));
  }
}

void AppToolTests::unknownToolAndBadArgsFailCleanly() {
  const OWzx::AppToolResult r = registry_->execute(
      QStringLiteral("definitely_not_a_tool"), {});
  QVERIFY2(!r.ok, "unknown tool must fail");
  QVERIFY(r.error.contains(QStringLiteral("Unknown tool")));
  QVERIFY(!registry_->hasTool(QStringLiteral("definitely_not_a_tool")));

  // Out-of-range object index on get_object_info.
  const OWzx::AppToolResult bad = registry_->execute(
      QStringLiteral("get_object_info"),
      QJsonObject{{QStringLiteral("index"), 9999}});
  QVERIFY2(!bad.ok, "out-of-range index must fail");
}

void AppToolTests::appStateSceneObjectInfoRoundTrip() {
  const OWzx::AppToolResult state =
      registry_->execute(QStringLiteral("get_app_state"), {});
  QVERIFY2(state.ok, "get_app_state must succeed");
  QVERIFY(state.data.value(QStringLiteral("objectCount")).toInt() >= 1);
  QVERIFY(!state.data.value(QStringLiteral("projectName")).toString().isEmpty());
  QVERIFY(state.data.contains(QStringLiteral("slice")));
  // Headless: no UI provider wired -> currentPage must be absent, and the UI
  // tools must fail with the documented "unavailable" error.
  QVERIFY(!state.data.contains(QStringLiteral("currentPage")));
  const OWzx::AppToolResult page = registry_->execute(
      QStringLiteral("switch_page"),
      QJsonObject{{QStringLiteral("page"), QStringLiteral("prepare")}});
  QVERIFY2(!page.ok, "switch_page must fail without a UI provider");
  QVERIFY(page.error.contains(QStringLiteral("headless")));

  const OWzx::AppToolResult scene =
      registry_->execute(QStringLiteral("get_scene"), {});
  QVERIFY2(scene.ok, "get_scene must succeed");
  const QJsonArray objects = scene.data.value(QStringLiteral("objects")).toArray();
  QVERIFY2(objects.size() >= 1, "scene must list the loaded object");
  const QJsonObject first = objects.at(0).toObject();
  QVERIFY(!first.value(QStringLiteral("name")).toString().isEmpty());
  QVERIFY(first.contains(QStringLiteral("position")));
  QVERIFY(first.contains(QStringLiteral("plateIndex")));
  QVERIFY(first.contains(QStringLiteral("printable")));

  const OWzx::AppToolResult info = registry_->execute(
      QStringLiteral("get_object_info"),
      QJsonObject{{QStringLiteral("index"), 0}});
  QVERIFY2(info.ok, "get_object_info must succeed for index 0");
  QVERIFY(info.data.contains(QStringLiteral("volumes")));
#ifdef HAS_LIBSLIC3R
  QVERIFY(info.data.value(QStringLiteral("triangleCount")).toInt() > 0);
#endif

  const OWzx::AppToolResult plates =
      registry_->execute(QStringLiteral("get_plate_info"), {});
  QVERIFY2(plates.ok, "get_plate_info must succeed");
  const QJsonArray plateArr =
      plates.data.value(QStringLiteral("plates")).toArray();
  QVERIFY2(plateArr.size() >= 1, "at least one plate must exist");
  const QJsonObject plate0 = plateArr.at(0).toObject();
  QVERIFY(plate0.value(QStringLiteral("objectIndices")).toArray().size() >= 1);
  QVERIFY(plate0.contains(QStringLiteral("readyForSlice")));

  const OWzx::AppToolResult sliceStatus =
      registry_->execute(QStringLiteral("get_slice_status"), {});
  QVERIFY2(sliceStatus.ok, "get_slice_status must succeed");
}

void AppToolTests::configListAndSetValueRoundTrip() {
  const OWzx::AppToolResult listing = registry_->execute(
      QStringLiteral("list_config_keys"),
      QJsonObject{{QStringLiteral("filter"), QStringLiteral("wall_loops")}});
  QVERIFY2(listing.ok, "list_config_keys must succeed");
  const QJsonArray keys = listing.data.value(QStringLiteral("keys")).toArray();
  QVERIFY2(keys.size() >= 1,
           "wall_loops must be discoverable via list_config_keys");
  const QJsonObject first = keys.at(0).toObject();
  QVERIFY(!first.value(QStringLiteral("key")).toString().isEmpty());
  QVERIFY(first.contains(QStringLiteral("currentValue")));
  QVERIFY(!first.value(QStringLiteral("tier")).toString().isEmpty());

  // Write through the same pipeline the settings UI uses.
  const OWzx::AppToolResult write = registry_->execute(
      QStringLiteral("set_config_value"),
      QJsonObject{{QStringLiteral("key"), QStringLiteral("wall_loops")},
                  {QStringLiteral("value"), 4}});
  QVERIFY2(write.ok, qPrintable(QStringLiteral("set_config_value failed: %1").arg(write.error)));

  // Read back through list_config_keys.
  const OWzx::AppToolResult after = registry_->execute(
      QStringLiteral("list_config_keys"),
      QJsonObject{{QStringLiteral("filter"), QStringLiteral("wall_loops")}});
  const QJsonValue current = after.data.value(QStringLiteral("keys")).toArray().at(0)
                                 .toObject().value(QStringLiteral("currentValue"));
  const QString asString = current.toString();
  QVERIFY2(asString == QLatin1String("4") ||
               QString::number(current.toInt()) == QLatin1String("4"),
           qPrintable(QStringLiteral("wall_loops should now be 4, got %1")
                          .arg(QString::fromUtf8(QJsonDocument(QJsonArray{current})
                                                     .toJson()))));

  // Unknown key fails with guidance.
  const OWzx::AppToolResult bad = registry_->execute(
      QStringLiteral("set_config_value"),
      QJsonObject{{QStringLiteral("key"), QStringLiteral("not_a_real_key_xyz")},
                  {QStringLiteral("value"), 1}});
  QVERIFY2(!bad.ok, "unknown config key must fail");
  QVERIFY(bad.error.contains(QStringLiteral("list_config_keys")));
}

void AppToolTests::transformDuplicateDeleteRoundTrip() {
  const int countBefore = project_->modelCount();

  const OWzx::AppToolResult dup = registry_->execute(
      QStringLiteral("duplicate_object"),
      QJsonObject{{QStringLiteral("index"), 0}});
  QVERIFY2(dup.ok, "duplicate_object must succeed");
  const int newIndex = dup.data.value(QStringLiteral("newIndex")).toInt();
  QVERIFY(newIndex >= 0);
  QCOMPARE(project_->modelCount(), countBefore + 1);

  const OWzx::AppToolResult moved = registry_->execute(
      QStringLiteral("set_object_transform"),
      QJsonObject{{QStringLiteral("index"), newIndex},
                  {QStringLiteral("position"),
                   QJsonObject{{QStringLiteral("x"), 50.0},
                               {QStringLiteral("y"), 0.0},
                               {QStringLiteral("z"), 50.0}}}});
  QVERIFY2(moved.ok, "set_object_transform must succeed");
  QCOMPARE(moved.data.value(QStringLiteral("position")).toObject()
               .value(QStringLiteral("x")).toDouble(), 50.0);

  const OWzx::AppToolResult sel = registry_->execute(
      QStringLiteral("select_object"),
      QJsonObject{{QStringLiteral("index"), newIndex}});
  QVERIFY2(sel.ok, "select_object must succeed");
  QCOMPARE(sel.data.value(QStringLiteral("selectedSourceIndex")).toInt(),
           newIndex);
  QCOMPARE(editor_->selectedSourceObjectIndex(), newIndex);

  const OWzx::AppToolResult del = registry_->execute(
      QStringLiteral("delete_object"),
      QJsonObject{{QStringLiteral("index"), newIndex}});
  QVERIFY2(del.ok, "delete_object must succeed");
  QCOMPARE(project_->modelCount(), countBefore);

  // The delete went through the undo stack — undo must restore it.
  const OWzx::AppToolResult undo = registry_->execute(QStringLiteral("undo"), {});
  QVERIFY2(undo.ok, "undo must succeed");
  QCOMPARE(project_->modelCount(), countBefore + 1);
  registry_->execute(QStringLiteral("redo"), {});  // restore cleaned state

  const OWzx::AppToolResult arranged = registry_->execute(
      QStringLiteral("arrange_objects"),
      QJsonObject{{QStringLiteral("spacingMm"), 10.0}});
  QVERIFY2(arranged.ok, "arrange_objects must succeed through the bed-shape path");
}

void AppToolTests::sliceGatesRejectInvalidRequests() {
  const OWzx::AppToolResult badPlate = registry_->execute(
      QStringLiteral("slice_plate"),
      QJsonObject{{QStringLiteral("plateIndex"), 999}});
  QVERIFY2(!badPlate.ok, "slice_plate must reject out-of-range plate");
  QVERIFY(badPlate.error.contains(QStringLiteral("out of range")));

  const OWzx::AppToolResult noResult = registry_->execute(
      QStringLiteral("export_gcode"),
      QJsonObject{{QStringLiteral("path"), QDir::temp().filePath(QStringLiteral("ai_never.gcode"))},
                  {QStringLiteral("plateIndex"), 0}});
  QVERIFY2(!noResult.ok, "export_gcode must reject a plate without a slice result");

  const OWzx::AppToolResult loadMissing = registry_->execute(
      QStringLiteral("load_model"),
      QJsonObject{{QStringLiteral("path"), QStringLiteral("Z:/no/such/file.stl")}});
  QVERIFY2(!loadMissing.ok, "load_model must reject a missing file");
}

void AppToolTests::mcpServerInitializeListAndCall() {
  OWzx::McpHttpServer server(registry_.get());
  QVERIFY2(server.start(/*port=*/0, kTestToken),
           "server must bind an ephemeral loopback port");
  const quint16 port = server.port();
  QVERIFY(port > 0);

  const QByteArray auth = "Bearer " + kTestToken.toUtf8();

  // initialize handshake
  const HttpResult init = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(QStringLiteral("initialize"),
                                   QJsonObject{{QStringLiteral("protocolVersion"),
                                                QStringLiteral("2025-06-18")}},
                                   1))
          .toJson(QJsonDocument::Compact),
      auth);
  QCOMPARE(init.status, 200);
  const QJsonObject initBody = QJsonDocument::fromJson(init.body).object();
  QCOMPARE(initBody.value(QStringLiteral("id")).toInt(), 1);
  const QJsonObject initResult = initBody.value(QStringLiteral("result")).toObject();
  QCOMPARE(initResult.value(QStringLiteral("protocolVersion")).toString(),
           QStringLiteral("2025-06-18"));
  QCOMPARE(initResult.value(QStringLiteral("serverInfo")).toObject()
               .value(QStringLiteral("name")).toString(),
           QStringLiteral("owzx-slicer"));
  QVERIFY(initResult.contains(QStringLiteral("capabilities")));

  // notifications (no id) are accepted with 202 and no body
  QJsonObject notif;
  notif.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
  notif.insert(QStringLiteral("method"), QStringLiteral("notifications/initialized"));
  const HttpResult notifRes = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(notif).toJson(QJsonDocument::Compact), auth);
  QCOMPARE(notifRes.status, 202);

  // tools/list mirrors the registry definitions
  const HttpResult list = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(QStringLiteral("tools/list"), {}, 2))
          .toJson(QJsonDocument::Compact),
      auth);
  QCOMPARE(list.status, 200);
  const QJsonArray listed = QJsonDocument::fromJson(list.body)
                                .object().value(QStringLiteral("result"))
                                .toObject().value(QStringLiteral("tools")).toArray();
  QCOMPARE(listed.size(), registry_->tools().size());

  // tools/call runs a real tool against the live stack
  const HttpResult call = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(
                        QStringLiteral("tools/call"),
                        QJsonObject{{QStringLiteral("name"), QStringLiteral("get_app_state")},
                                    {QStringLiteral("arguments"), QJsonObject{}}},
                        3))
          .toJson(QJsonDocument::Compact),
      auth);
  QCOMPARE(call.status, 200);
  const QJsonObject callResult = QJsonDocument::fromJson(call.body)
                                     .object().value(QStringLiteral("result")).toObject();
  QCOMPARE(callResult.value(QStringLiteral("isError")).toBool(), false);
  const QString text = callResult.value(QStringLiteral("content")).toArray().at(0)
                           .toObject().value(QStringLiteral("text")).toString();
  const QJsonObject toolPayload = QJsonDocument::fromJson(text.toUtf8()).object();
  QVERIFY2(toolPayload.value(QStringLiteral("ok")).toBool() == true,
           "tool payload envelope must carry ok:true");
  QVERIFY(toolPayload.value(QStringLiteral("objectCount")).toInt() >= 1);
}

void AppToolTests::mcpServerAuthAndErrorRouting() {
  OWzx::McpHttpServer server(registry_.get());
  QVERIFY(server.start(0, kTestToken));
  const quint16 port = server.port();

  // Wrong token -> 401
  const HttpResult badAuth = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(QStringLiteral("tools/list"), {}, 9))
          .toJson(QJsonDocument::Compact),
      "Bearer wrong-token");
  QCOMPARE(badAuth.status, 401);

  // Non-POST -> 405
  QCOMPARE(httpPost(port, "GET", "/mcp", {},
                    "Bearer " + kTestToken.toUtf8()).status, 405);
  // Wrong path -> 404
  QCOMPARE(httpPost(port, "POST", "/other", {},
                    "Bearer " + kTestToken.toUtf8()).status, 404);
  // Non-loopback Host header (DNS rebinding) -> 403
  QCOMPARE(httpPost(port, "POST", "/mcp", {},
                    "Bearer " + kTestToken.toUtf8(),
                    /*hostOverride=*/"evil.example.com").status, 403);

  // Unknown JSON-RPC method -> -32601
  const HttpResult badMethod = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(QStringLiteral("bogus/method"), {}, 10))
          .toJson(QJsonDocument::Compact),
      "Bearer " + kTestToken.toUtf8());
  QCOMPARE(badMethod.status, 200);
  const QJsonObject errBody = QJsonDocument::fromJson(badMethod.body).object();
  QCOMPARE(errBody.value(QStringLiteral("error")).toObject()
               .value(QStringLiteral("code")).toInt(), -32601);

  // Unknown tool name -> -32602 (MCP spec)
  const HttpResult badTool = httpPost(
      port, "POST", "/mcp",
      QJsonDocument(jsonRpcRequest(
                        QStringLiteral("tools/call"),
                        QJsonObject{{QStringLiteral("name"), QStringLiteral("nope")},
                                    {QStringLiteral("arguments"), QJsonObject{}}},
                        11))
          .toJson(QJsonDocument::Compact),
      "Bearer " + kTestToken.toUtf8());
  QCOMPARE(badTool.status, 200);
  QCOMPARE(QJsonDocument::fromJson(badTool.body).object()
               .value(QStringLiteral("error")).toObject()
               .value(QStringLiteral("code")).toInt(), -32602);
}

QTEST_MAIN(AppToolTests)
#include "AppToolTests.moc"
