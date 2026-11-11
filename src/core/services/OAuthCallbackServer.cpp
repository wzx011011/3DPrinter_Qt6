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
    : QObject(parent)
{
    ensureServer();
}

// OWzx: fresh QTcpServer per listen session. Qt 6.12/macOS: close() inside
// the newConnection/readyRead activation flips isListening() but LEAVES THE
// LISTEN FD ACCEPTING (a loopback connect still reached ConnectedState one
// second after close()) — destroying the QTcpServer object runs its engine
// destructor, which actually closes the native socket.
void OAuthCallbackServer::ensureServer()
{
    if (server_)
        return;
    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this,
            &OAuthCallbackServer::handleConnection);
}

OAuthCallbackServer::~OAuthCallbackServer() = default;

bool OAuthCallbackServer::start(int port)
{
    ensureServer();
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
    if (!server_)
        return;
    // OWzx: destroy the whole QTcpServer WITHOUT aborting the accepted
    // child sockets — their queued responses (the 302 bounce) must flush
    // first; each socket closes itself via disconnectFromHost ->
    // deleteLater. Destroying the QTcpServer is the point: Qt 6.12/macOS
    // close() inside the newConnection/readyRead activation flips
    // isListening() but leaves the listen fd accepting (verified: a
    // loopback connect reached ConnectedState one second after close()).
    // The engine destructor actually closes the native socket. The next
    // start() builds a fresh object.
    server_->deleteLater();
    server_ = nullptr;
    emit listeningChanged();
}

bool OAuthCallbackServer::listening() const
{
    return server_ && server_->isListening();
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
    if (codeCaptured_) {
        // Capture-once: the port may still be answering at the TCP layer
        // after stop() on macOS — refuse everything from here on.
        while (QTcpSocket *stale = server_->nextPendingConnection())
            stale->abort();
        return;
    }
    while (QTcpSocket *socket = server_->nextPendingConnection()) {
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        // The request is complete when the blank line terminates the headers;
        // there is never a request body on this endpoint.
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            // C12: accumulate per socket -- a request line split across TCP
            // segments must not lose its first part.
            QByteArray raw = socket->property("owzxRequest").toByteArray();
            raw += socket->readAll();
            if (raw.size() > 16 * 1024) {
                socket->abort();  // not an oauth redirect navigation
                return;
            }
            socket->setProperty("owzxRequest", raw);
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
            if (!code.isEmpty()) {
                codeCaptured_ = true;
                // C12: one code per login attempt. The callback protocol
                // carries no OAuth `state` (the public oauth page only
                // echoes back_url), so the listener stops at the first code
                // and later requests -- e.g. a forged /login?code= from
                // another local page -- find no listener.
                stop();
                emit codeReceived(code);
            }
        });
    }
}
