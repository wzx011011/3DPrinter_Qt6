// McpHttpServer implementation — MCP Streamable HTTP subset on loopback.
// See McpHttpServer.h for the protocol/security contract.

#include "McpHttpServer.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace OWzx {
namespace {
constexpr const char *kServerName = "owzx-slicer";
constexpr const char *kServerVersion = "1.0";
constexpr const char *kDefaultProtocolVersion = "2025-06-18";
constexpr int kMaxRequestBytes = 4 * 1024 * 1024;  // 4 MiB request cap

QByteArray statusText(int code) {
  switch (code) {
    case 200: return "OK";
    case 202: return "Accepted";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    default: return "Error";
  }
}

QString headerValue(const QByteArray &headers, const char *name) {
  // Case-insensitive single-value header lookup over raw "k: v\r\n" lines.
  const QByteArray needle = QByteArray(name).toLower() + ':';
  const QList<QByteArray> lines = headers.split('\n');
  for (const QByteArray &raw : lines) {
    QByteArray line = raw.trimmed();
    if (line.toLower().startsWith(needle)) {
      return QString::fromLatin1(line.mid(int(needle.size())).trimmed());
    }
  }
  return QString();
}
}  // namespace

McpHttpServer::McpHttpServer(AppToolRegistry *registry, QObject *parent)
    : QObject(parent), registry_(registry) {
  connect(&server_, &QTcpServer::newConnection, this,
          &McpHttpServer::onNewConnection);
}

bool McpHttpServer::start(quint16 port, const QString &token) {
  stop();
  if (!registry_ || token.isEmpty())
    return false;
  token_ = token;
  if (!server_.listen(QHostAddress::LocalHost, port))
    return false;
  emit started();
  return true;
}

void McpHttpServer::stop() {
  if (server_.isListening()) {
    server_.close();
    emit stopped();
  }
}

void McpHttpServer::onNewConnection() {
  while (server_.hasPendingConnections()) {
    QTcpSocket *socket = server_.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this,
            [this, socket]() { onSocketReadyRead(socket); });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  }
}

void McpHttpServer::onSocketReadyRead(QTcpSocket *socket) {
  // One request per connection (Connection: close) — accumulate until the
  // full body per Content-Length is available, then dispatch.
  QByteArray buf = socket->property("buf").toByteArray();
  buf += socket->readAll();
  if (buf.size() > kMaxRequestBytes) {
    sendHttpResponse(socket, 413, {});
    socket->disconnectFromHost();
    return;
  }
  const int headerEnd = buf.indexOf("\r\n\r\n");
  if (headerEnd < 0) {
    if (buf.size() > 32 * 1024) {  // header section alone is oversized
      sendHttpResponse(socket, 400, {});
      socket->disconnectFromHost();
    } else {
      socket->setProperty("buf", buf);
    }
    return;
  }
  const QByteArray headers = buf.left(headerEnd);
  const QByteArray body = buf.mid(headerEnd + 4);

  bool ok = false;
  const int contentLength = headerValue(headers, "Content-Length").toInt(&ok);
  if (!ok || contentLength < 0 || body.size() < contentLength) {
    socket->setProperty("buf", buf);  // wait for the rest
    return;
  }
  if (handleRequest(socket, headers, body.left(contentLength))) {
    socket->disconnectFromHost();
  }
}

bool McpHttpServer::handleRequest(QTcpSocket *socket, const QByteArray &headers,
                                  const QByteArray &body) {
  const QList<QByteArray> requestLine = headers.split('\r').first().split(' ');
  if (requestLine.size() < 2) {
    sendHttpResponse(socket, 400, {});
    return true;
  }
  const QByteArray httpMethod = requestLine.at(0).trimmed();
  const QByteArray path = requestLine.at(1).trimmed();

  // DNS-rebinding guard: Host must be loopback; Origin, when present, too.
  const QString host = headerValue(headers, "Host");
  const QString origin = headerValue(headers, "Origin");
  const bool hostOk = host.startsWith(QLatin1String("127.0.0.1")) ||
                      host.startsWith(QLatin1String("localhost")) ||
                      host.startsWith(QLatin1String("[::1]"));
  const bool originOk = origin.isEmpty() ||
                        origin.startsWith(QLatin1String("http://127.0.0.1")) ||
                        origin.startsWith(QLatin1String("http://localhost"));
  if (!hostOk || !originOk) {
    sendHttpResponse(socket, 403, {});
    return true;
  }

  if (httpMethod != "POST") {
    sendHttpResponse(socket, 405, {});
    return true;
  }
  if (path != "/mcp") {
    sendHttpResponse(socket, 404, {});
    return true;
  }
  const QString auth = headerValue(headers, "Authorization");
  if (auth != QStringLiteral("Bearer ") + token_) {
    sendHttpResponse(socket, 401, {});
    return true;
  }

  const QJsonDocument doc = QJsonDocument::fromJson(body);
  if (!doc.isObject()) {
    sendHttpResponse(socket, 400, {});
    return true;
  }

  const QJsonObject request = doc.object();
  const QString method = request.value(QStringLiteral("method")).toString();

  // MCP notifications (no "id") get no response body.
  if (!request.contains(QStringLiteral("id"))) {
    emit requestLogged(method, QString(), true);
    sendHttpResponse(socket, 202, {});
    return true;
  }

  bool isError = false;
  int errorCode = 0;
  QString errorMessage;
  const QJsonObject payload =
      dispatch(request, &isError, &errorCode, &errorMessage);
  QJsonObject response;
  response.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
  response.insert(QStringLiteral("id"), request.value(QStringLiteral("id")));
  if (isError) {
    QJsonObject err;
    err.insert(QStringLiteral("code"), errorCode);
    err.insert(QStringLiteral("message"), errorMessage);
    response.insert(QStringLiteral("error"), err);
  } else {
    response.insert(QStringLiteral("result"), payload);
  }
  sendHttpResponse(socket, 200,
                   QJsonDocument(response).toJson(QJsonDocument::Compact));
  return true;
}

QJsonObject McpHttpServer::dispatch(const QJsonObject &request, bool *isError,
                                    int *errorCode, QString *errorMessage) {
  *isError = false;
  const QString method = request.value(QStringLiteral("method")).toString();
  const QJsonObject params = request.value(QStringLiteral("params")).toObject();

  if (method == QStringLiteral("initialize")) {
    QString protocol =
        params.value(QStringLiteral("protocolVersion")).toString();
    if (protocol.isEmpty())
      protocol = QString::fromLatin1(kDefaultProtocolVersion);
    QJsonObject result;
    result.insert(QStringLiteral("protocolVersion"), protocol);
    QJsonObject capsTools;
    capsTools.insert(QStringLiteral("listChanged"), false);
    QJsonObject caps;
    caps.insert(QStringLiteral("tools"), capsTools);
    result.insert(QStringLiteral("capabilities"), caps);
    QJsonObject info;
    info.insert(QStringLiteral("name"), QString::fromLatin1(kServerName));
    info.insert(QStringLiteral("version"), QString::fromLatin1(kServerVersion));
    result.insert(QStringLiteral("serverInfo"), info);
    emit requestLogged(method, QString(), true);
    return result;
  }

  if (method == QStringLiteral("tools/list")) {
    QJsonObject result;
    result.insert(QStringLiteral("tools"), registry_->toolDefinitions());
    emit requestLogged(method, QString(), true);
    return result;
  }

  if (method == QStringLiteral("tools/call")) {
    const QString name = params.value(QStringLiteral("name")).toString();
    const QJsonObject arguments =
        params.value(QStringLiteral("arguments")).toObject();
    if (!registry_->hasTool(name)) {
      *isError = true;
      *errorCode = -32602;  // Invalid params: unknown tool (per MCP spec)
      *errorMessage = QStringLiteral("Unknown tool: %1").arg(name);
      emit requestLogged(method, name, false);
      return {};
    }
    const AppToolResult result = registry_->execute(name, arguments);
    emit requestLogged(method, name, result.ok);
    // MCP tools/call result: text content carrying the tool JSON envelope.
    QJsonObject contentItem;
    contentItem.insert(QStringLiteral("type"), QStringLiteral("text"));
    contentItem.insert(QStringLiteral("text"),
                       QString::fromUtf8(QJsonDocument(result.toJson())
                                             .toJson(QJsonDocument::Compact)));
    QJsonObject out;
    out.insert(QStringLiteral("content"),
               QJsonArray{contentItem});
    out.insert(QStringLiteral("isError"), !result.ok);
    return out;
  }

  if (method == QStringLiteral("ping")) {
    emit requestLogged(method, QString(), true);
    return {};
  }

  *isError = true;
  *errorCode = -32601;  // Method not found
  *errorMessage = QStringLiteral("Method not supported: %1").arg(method);
  emit requestLogged(method, QString(), false);
  return {};
}

void McpHttpServer::sendHttpResponse(QTcpSocket *socket, int status,
                                     const QByteArray &body) {
  QByteArray head;
  head += "HTTP/1.1 " + QByteArray::number(status) + " " + statusText(status) + "\r\n";
  head += "Content-Type: application/json\r\n";
  head += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
  head += "Connection: close\r\n\r\n";
  socket->write(head + body);
}

}  // namespace OWzx
