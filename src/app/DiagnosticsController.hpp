// src/app/DiagnosticsController.hpp
// 诊断页网络探测后端 / Diagnostic probe backend for the diagnostics page.
// 把原本落在 QML XMLHttpRequest 的 HTTP 短探测、DoH 对比、门户页编码嗅探收拢到
// QNAM 统一管理；QML 只订阅结构化结果并渲染文案，不直接发请求 / 解析响应。
// Moves HTTP probe, DoH comparison and charset sniffing off the QML side into a
// single QNAM; QML subscribes to structured results and renders the labels only.
#ifndef GDEI_DIAGNOSTICS_CONTROLLER_HPP
#define GDEI_DIAGNOSTICS_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;

namespace gdei {

class ConfigStore;

class DiagnosticsController : public QObject {
    Q_OBJECT
public:
    explicit DiagnosticsController(ConfigStore* config, QObject* parent = nullptr);

    // HTTP HEAD 短探测；结果经 httpProbeFinished 回传。
    Q_INVOKABLE void probeHttp();
    // DoH 并行延迟对比；结果经 dohProbeFinished 回传。
    Q_INVOKABLE void probeDoh();
    // 门户页 charset 嗅探；结果经 encodingFinished 回传。
    Q_INVOKABLE void sniffCharset();

signals:
    // HTTP 探测结果：ok 为是否收到任意响应（status>0）；status 为 HTTP 状态码；
    // ms 为耗时；url 为实际请求地址；error 为 timeout / network / 空串。
    // ok means any response arrived (status>0); error categorizes the failure.
    void httpProbeFinished(bool ok, int status, qint64 ms, QString url, QString error);
    // DoH 聚合结果：[{ name, ok, ms, extra }]，两端点全部结束后发出一次。
    void dohProbeFinished(QVariantList results);
    // 编码嗅探结果：charset 为大写编码名（嗅不到为空）；url 为实际请求地址；
    // ok 为是否嗅探到编码；error 为 timeout / network / 空串。
    void encodingFinished(bool ok, QString charset, QString url, QString error);

private:
    struct DohEntry {
        QString name;
        QString url;
        QNetworkReply* reply = nullptr;
        qint64 t0 = 0;
        bool settled = false;
        bool ok = false;
        qint64 ms = 0;
        QString extra;
    };

    ConfigStore* m_config;
    QNetworkAccessManager* m_nam;
    QVariantList m_dohResults;
    int m_dohPending = 0;

    int probeTimeoutMs() const;
    static QString sniffCharsetFrom(const QByteArray& contentType, const QByteArray& body);
};

} // namespace gdei

#endif // GDEI_DIAGNOSTICS_CONTROLLER_HPP
