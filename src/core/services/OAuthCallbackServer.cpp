#include "OAuthCallbackServer.h"

#include <QTcpSocket>
#include <QUrlQuery>

namespace {
/// Extract ?code= from a request target like "/login?code=abc&result_url=…".
QString extractCodeFromTarget(const QString &target)
{
    const int queryStart = int(target.indexOf(QLatin1Char('?')));
    if (queryStart == -1)
        return {};
    QUrlQuery query{target.mid(queryStart + 1)};
    return query.queryItemValue(QStringLiteral("code"));
}

/// Minimal one-file 302 response (request body is never read; the oauth
/// redirect navigation carries everything in the URL).
const char kRedirectHead[] =
    "HTTP/1.1 302 Found\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "Location: ";
const char kNotFoundHead[] =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n\r\n";
} // namespace

OAuthCallbackServer::OAuthCallbackServer(QObject *parent)
    : QObject(parent), server_(new QTcpServer(this))
{
    connect(server_, &QTcpServer::newConnection, this,
            &OAuthCallbackServer::handleConnection);
}

OAuthCallbackServer::~OAuthCallbackServer() = default;

bool OAuthCallbackServer::start(int port)
{
    if (server_->isListening())
        return true;
    const bool ok = server_->listen(QHostAddress::LocalHost, quint16(port));
    if (!ok)
        emit errorOccurred(server_->errorString());
    emit listeningChanged();
    return ok;
}

void OAuthCallbackServer::stop()
{
    if (!server_->isListening())
        return;
    server_->close();
    emit listeningChanged();
}

bool OAuthCallbackServer::listening() const
{
    return server_->isListening();
}

void OAuthCallbackServer::setSuccessRedirectUrl(const QString &url)
{
    successRedirectUrl_ = url;
}

void OAuthCallbackServer::setFailureRedirectUrl(const QString &url)
{
    failureRedirectUrl_ = url;
}

void OAuthCallbackServer::handleConnection()
{
    while (QTcpSocket *socket = server_->nextPendingConnection()) {
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        // The request is complete when the blank line terminates the headers;
        // there is never a request body on this endpoint.
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QByteArray raw = socket->readAll();
            const QString request = QString::fromLatin1(raw);
            const int lineEnd = int(request.indexOf(QStringLiteral("\r\n")));
            if (lineEnd == -1)
                return; // keep buffering until the request line is complete
            const QString requestLine = request.left(lineEnd);
            const QStringList parts = requestLine.split(QLatin1Char(' '));
            // "GET /login?code=… HTTP/1.1"
            if (parts.size() < 2 || !parts.first().startsWith(QStringLiteral("GET"))) {
                socket->write(kNotFoundHead, int(qstrlen(kNotFoundHead)));
                socket->disconnectFromHost();
                return;
            }
            const QString target = parts.at(1);
            if (!target.startsWith(QStringLiteral("/login"))) {
                socket->write(kNotFoundHead, int(qstrlen(kNotFoundHead)));
                socket->disconnectFromHost();
                return;
            }
            const QString code = extractCodeFromTarget(target);
            const QByteArray &bounce =
                code.isEmpty() ? failureRedirectUrl_.toUtf8() : successRedirectUrl_.toUtf8();
            socket->write(kRedirectHead, int(qstrlen(kRedirectHead)));
            socket->write(bounce);
            socket->write("\r\n\r\n", 4);
            socket->disconnectFromHost();
            if (!code.isEmpty())
                emit codeReceived(code);
        });
    }
}
