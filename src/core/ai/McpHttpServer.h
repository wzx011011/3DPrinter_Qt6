#pragma once

// McpHttpServer — OWzx-only AI control transport (decision record:
// docs/ai-control.md).
//
// Minimal MCP (Model Context Protocol) server over Streamable HTTP on
// 127.0.0.1, exposing an AppToolRegistry to out-of-process agent harnesses
// (the embedded Claude Agent SDK sidecar) and, if enabled, external MCP
// clients. Implements the stable core subset every MCP client supports:
//   POST /mcp  initialize | tools/list | tools/call | ping   (JSON responses)
//   notifications (no "id") get 202 with no body.
//
// Security: loopback bind only, Bearer token check, Host/Origin validation
// against DNS rebinding. All tool execution happens on the GUI thread (this
// server must be created and started there; sockets signal on that thread).

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "AppToolRegistry.h"

namespace OWzx {

class McpHttpServer final : public QObject {
  Q_OBJECT

 public:
  explicit McpHttpServer(AppToolRegistry *registry, QObject *parent = nullptr);

  /// Binds 127.0.0.1:port. Returns false when the port is taken.
  bool start(quint16 port, const QString &token);
  void stop();
  bool isActive() const { return server_.isListening(); }
  quint16 port() const { return server_.serverPort(); }

 signals:
  void started();
  void stopped();
  /// Observability hook for the AI sidebar / logs: every JSON-RPC request.
  void requestLogged(const QString &method, const QString &tool, bool ok);

 private:
  void onNewConnection();
  void onSocketReadyRead(QTcpSocket *socket);
  bool handleRequest(QTcpSocket *socket, const QByteArray &header,
                     const QByteArray &body);
  // JSON-RPC dispatch; returns result object for the response body.
  QJsonObject dispatch(const QJsonObject &request, bool *isError,
                       int *errorCode, QString *errorMessage);
  static void sendHttpResponse(QTcpSocket *socket, int status,
                               const QByteArray &body);

  AppToolRegistry *registry_ = nullptr;
  QTcpServer server_;
  QString token_;
};

}  // namespace OWzx
