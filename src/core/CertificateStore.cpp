// src/core/CertificateStore.cpp
#include "core/CertificateStore.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <utility>

namespace gdei {

CertificateStore::CertificateStore(QObject* parent) : QObject(parent) {
    m_dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                .filePath(QStringLiteral("ca"));
    reload();
}

QString CertificateStore::fingerprintOf(const QSslCertificate& certificate) {
    return QString::fromLatin1(
        certificate.digest(QCryptographicHash::Sha256).toHex());
}

QString CertificateStore::filePathFor(const QSslCertificate& certificate) const {
    // 用指纹前 32 位做文件名，天然去重且不含用户输入。
    return QDir(m_dir).filePath(fingerprintOf(certificate).left(32) + QStringLiteral(".pem"));
}

void CertificateStore::reload() {
    m_certificates.clear();
    QDir dir(m_dir);
    const QStringList files = dir.entryList({QStringLiteral("*.pem"), QStringLiteral("*.crt")},
                                            QDir::Files | QDir::Readable);
    for (const QString& name : files) {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::ReadOnly)) continue;
        const QSslCertificate certificate(file.readAll(), QSsl::Pem);
        if (certificate.isNull()) continue;
        const QString fp = fingerprintOf(certificate);
        bool duplicate = false;
        for (const QSslCertificate& existing : std::as_const(m_certificates)) {
            if (fingerprintOf(existing) == fp) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) m_certificates.append(certificate);
    }
}

QVariantList CertificateStore::certificates() const {
    QVariantList list;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    list.reserve(m_certificates.size());
    for (const QSslCertificate& certificate : m_certificates) {
        QVariantMap item;
        item.insert(QStringLiteral("subject"), certificate.subjectDisplayName());
        item.insert(QStringLiteral("issuer"), certificate.issuerDisplayName());
        item.insert(QStringLiteral("notBefore"),
                    certificate.effectiveDate().toUTC().toString(Qt::ISODate));
        item.insert(QStringLiteral("notAfter"),
                    certificate.expiryDate().toUTC().toString(Qt::ISODate));
        item.insert(QStringLiteral("expired"), certificate.expiryDate() < now);
        item.insert(QStringLiteral("fingerprint"), fingerprintOf(certificate));
        list.append(item);
    }
    return list;
}

bool CertificateStore::persist(const QSslCertificate& certificate) {
    if (!QDir().mkpath(m_dir)) return false;
    QFile file(filePathFor(certificate));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    // 统一写规范 PEM，避免原文件编码/换行差异。
    return file.write(certificate.toPem()) > 0;
}

int CertificateStore::importFromPath(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return 1;
    const QByteArray data = file.readAll();
    file.close();
    if (data.isEmpty()) return 1;

    // 兼容 PEM 与 DER：先按 PEM 解析，失败再按 DER。
    QSslCertificate certificate(data, QSsl::Pem);
    if (certificate.isNull()) certificate = QSslCertificate(data, QSsl::Der);
    if (certificate.isNull()) return 2;

    const QString fp = fingerprintOf(certificate);
    for (const QSslCertificate& existing : std::as_const(m_certificates)) {
        if (fingerprintOf(existing) == fp) {
            emit changed();  // 已存在：视为成功并刷新列表
            return 0;
        }
    }
    if (!persist(certificate)) return 3;
    m_certificates.append(certificate);
    emit changed();
    return 0;
}

bool CertificateStore::removeByFingerprint(const QString& fingerprint) {
    for (int i = 0; i < m_certificates.size(); ++i) {
        if (fingerprintOf(m_certificates.at(i)) != fingerprint) continue;
        QFile::remove(filePathFor(m_certificates.at(i)));
        m_certificates.removeAt(i);
        emit changed();
        return true;
    }
    return false;
}

} // namespace gdei
