// src/core/CertificateStore.hpp
// 用户 CA 信任管理 / User CA trust store.
//
// 门户证书通常由学校自签 CA 签发。这里提供一个轻量、可审计的信任锚目录：
//   - 导入 PEM 证书后保存到应用数据目录的 ca/ 下；
//   - PortalClient 只把这些证书追加进 QSslConfiguration，不放宽任何校验；
//   - 列表展示主体、有效期与 SHA-256 指纹，过期项显式标记。
// Lightweight CA trust anchors: import PEM/DER, persist under AppData/ca, and let
// PortalClient append them to QSslConfiguration without weakening verification.
#ifndef GDEI_CERTIFICATE_STORE_HPP
#define GDEI_CERTIFICATE_STORE_HPP

#include <QList>
#include <QObject>
#include <QSslCertificate>
#include <QString>
#include <QVariantList>

namespace gdei {

class CertificateStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList certificates READ certificates NOTIFY changed)
public:
    explicit CertificateStore(QObject* parent = nullptr);

    QString directory() const { return m_dir; }
    QList<QSslCertificate> trustedCertificates() const { return m_certificates; }
    QVariantList certificates() const;

    // 返回码：0 成功，1 读取失败，2 不是有效证书，3 写入失败。
    // Returns 0 ok, 1 read failure, 2 invalid certificate, 3 write failure.
    Q_INVOKABLE int importFromPath(const QString& path);
    Q_INVOKABLE bool removeByFingerprint(const QString& fingerprint);

signals:
    void changed();

private:
    void reload();
    bool persist(const QSslCertificate& certificate);
    QString filePathFor(const QSslCertificate& certificate) const;
    static QString fingerprintOf(const QSslCertificate& certificate);

    QString m_dir;
    QList<QSslCertificate> m_certificates;
};

} // namespace gdei

#endif // GDEI_CERTIFICATE_STORE_HPP
