#include "app/DiagnosticsController.hpp"

#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"

#include <QDateTime>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>
#include <QVariantMap>

namespace gdei {

DiagnosticsController::DiagnosticsController(ConfigStore* config, QObject* parent)
    : QObject(parent), m_config(config) {
    // 诊断为公开只读探测：使用独立 QNAM，不带认证 CookieJar，与会话隔离；
    // 证书默认严格校验，不跟随“跳过证书校验”开关，握手错误经 reply->error() 上报。
    // Public read-only probes: isolated QNAM without the session cookie jar;
    // certificate verification stays strict and does not follow the insecure toggle.
    m_nam = new QNetworkAccessManager(this);
}

int DiagnosticsController::probeTimeoutMs() const {
    int ms = m_config ? m_config->probeTimeoutMs() : 0;
    return qMax(1000, ms);
}

// ── HTTP HEAD 短探测 ──
void DiagnosticsController::probeHttp() {
    QString url = m_config && !m_config->probeUrl().isEmpty()
                      ? m_config->probeUrl()
                      : constants::DIAG_HTTP_FALLBACK_URL;

    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(probeTimeoutMs());

    QNetworkReply* reply = m_nam->head(req);
    QElapsedTimer timer;
    timer.start();

    connect(reply, &QNetworkReply::finished, this, [this, reply, url, timer]() {
        const qint64 ms = timer.elapsed();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto err = reply->error();

        QString error;
        bool ok = false;
        if (status > 0) {
            ok = true;
        } else if (err == QNetworkReply::TimeoutError) {
            error = QStringLiteral("timeout");
        } else {
            error = QStringLiteral("network");
        }

        emit httpProbeFinished(ok, status, ms, url, error);
        reply->deleteLater();
    });
}

// ── DoH 并行延迟对比 ──
void DiagnosticsController::probeDoh() {
    m_dohResults.clear();
    m_dohPending = 0;

    const QList<DohEntry> seeds = {
        {QStringLiteral("AliDNS"), constants::DOH_ALI_ENDPOINT, nullptr, 0, false, false, 0, QString()},
        {QStringLiteral("Cloudflare"), constants::DOH_CF_ENDPOINT, nullptr, 0, false, false, 0, QString()},
    };

    for (const auto& seed : seeds) {
        DohEntry entry = seed;
        QNetworkRequest req{QUrl(entry.url)};
        req.setTransferTimeout(probeTimeoutMs());
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/dns-json"));
        req.setRawHeader("Accept", "application/dns-json");

        entry.reply = m_nam->get(req);
        entry.t0 = QDateTime::currentMSecsSinceEpoch();
        ++m_dohPending;

        QNetworkReply* reply = entry.reply;
        connect(reply, &QNetworkReply::finished, this, [this, entry, reply]() mutable {
            if (entry.settled) return;
            entry.settled = true;
            entry.ms = QDateTime::currentMSecsSinceEpoch() - entry.t0;

            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const auto err = reply->error();
            entry.ok = (status >= 200 && status < 500);
            if (status > 0) {
                entry.extra = QStringLiteral("HTTP %1").arg(status);
            } else if (err == QNetworkReply::TimeoutError) {
                entry.extra = QStringLiteral("timeout");
            } else {
                entry.extra = QStringLiteral("error");
            }

            QVariantMap row;
            row[QStringLiteral("name")] = entry.name;
            row[QStringLiteral("ok")] = entry.ok;
            row[QStringLiteral("ms")] = entry.ms;
            row[QStringLiteral("extra")] = entry.extra;
            m_dohResults.append(row);

            --m_dohPending;
            if (m_dohPending <= 0) {
                emit dohProbeFinished(m_dohResults);
            }
            reply->deleteLater();
        });
    }
}

// ── 门户页 charset 嗅探 ──
void DiagnosticsController::sniffCharset() {
    const QString host = m_config && !m_config->host().isEmpty()
                             ? m_config->host()
                             : constants::DIAG_PORTAL_HOST;
    const QString url = host.indexOf("://") >= 0 ? host : QStringLiteral("http://") + host + "/";

    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(probeTimeoutMs());

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        const QByteArray contentType =
            reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        QByteArray body = reply->readAll();
        if (body.size() > 4000) body = body.left(4000);

        const QString charset = sniffCharsetFrom(contentType, body);
        QString error;
        if (charset.isEmpty()) {
            const auto err = reply->error();
            if (err == QNetworkReply::TimeoutError) {
                error = QStringLiteral("timeout");
            } else if (err != QNetworkReply::NoError) {
                error = QStringLiteral("network");
            }
        }
        emit encodingFinished(!charset.isEmpty(), charset, url, error);
        reply->deleteLater();
    });
}

QString DiagnosticsController::sniffCharsetFrom(const QByteArray& contentType,
                                                 const QByteArray& body) {
    // 优先级：HTTP Content-Type charset → HTML meta charset；嗅不到返回空（不兜底）。
    // Priority: Content-Type charset → HTML meta charset; empty when neither declares it.
    QRegularExpression reHdr("charset=([^\\s;]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch mh = reHdr.match(QString::fromLatin1(contentType));
    if (mh.hasMatch()) {
        return mh.captured(1).toUpper();
    }

    QRegularExpression reMeta(
        "<meta[^>]+charset=[\"']?([^\\s\"'>/]+)",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch mm = reMeta.match(QString::fromUtf8(body));
    if (mm.hasMatch()) {
        return mm.captured(1).toUpper();
    }

    return QString();
}

} // namespace gdei
