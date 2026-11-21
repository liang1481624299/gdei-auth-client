// src/core/ConfigStore.hpp
// 配置持久化 / Persistent settings via QSettings.
// 口令仅在“记住密码”开启时经平台安全存储（SecretStore）保存，不明文写入 QSettings。
// Passwords go through the platform secure store (SecretStore) only when
// "remember password" is on; they are never written to QSettings in plaintext.
#ifndef GDEI_CONFIG_STORE_HPP
#define GDEI_CONFIG_STORE_HPP

#include "core/Types.hpp"
#include "core/Constants.hpp"

#include <QObject>
#include <QString>

namespace gdei {

class ConfigStore : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool rememberPassword READ rememberPassword WRITE setRememberPassword NOTIFY rememberPasswordChanged)
    Q_PROPERTY(bool autoLogin READ autoLogin WRITE setAutoLogin NOTIFY autoLoginChanged)
    Q_PROPERTY(int lang READ lang WRITE setLang NOTIFY langChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(QString ip READ ip WRITE setIp NOTIFY ipChanged)
    Q_PROPERTY(QString nasip READ nasip WRITE setNasip NOTIFY nasipChanged)
    Q_PROPERTY(bool allowInsecureSsl READ allowInsecureSsl WRITE setAllowInsecureSsl NOTIFY allowInsecureSslChanged)
    Q_PROPERTY(int keepaliveMinSec READ keepaliveMinSec WRITE setKeepaliveMinSec NOTIFY keepaliveMinSecChanged)
    Q_PROPERTY(int keepaliveMaxSec READ keepaliveMaxSec WRITE setKeepaliveMaxSec NOTIFY keepaliveMaxSecChanged)
    Q_PROPERTY(int offlineFailThreshold READ offlineFailThreshold WRITE setOfflineFailThreshold NOTIFY offlineFailThresholdChanged)

public:
    explicit ConfigStore(QObject* parent = nullptr);

    QString username() const;
    QString password() const;
    bool rememberPassword() const;
    bool autoLogin() const;
    int lang() const;
    QString host() const;
    QString ip() const;
    QString nasip() const;
    bool allowInsecureSsl() const;
    int keepaliveMinSec() const;
    int keepaliveMaxSec() const;
    int offlineFailThreshold() const;

    void setUsername(const QString& v);
    void setPassword(const QString& v);
    void setRememberPassword(bool v);
    void setAutoLogin(bool v);
    void setLang(int v);
    void setHost(const QString& v);
    void setIp(const QString& v);
    void setNasip(const QString& v);
    void setAllowInsecureSsl(bool v);
    void setKeepaliveMinSec(int v);
    void setKeepaliveMaxSec(int v);
    void setOfflineFailThreshold(int v);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

signals:
    void usernameChanged();
    void passwordChanged();
    void rememberPasswordChanged();
    void autoLoginChanged();
    void langChanged();
    void hostChanged();
    void ipChanged();
    void nasipChanged();
    void allowInsecureSslChanged();
    void keepaliveMinSecChanged();
    void keepaliveMaxSecChanged();
    void offlineFailThresholdChanged();

private:
    QString m_username;
    QString m_password;        // 内存中的明文，不落盘 / plaintext in memory only
    bool m_rememberPassword = false;
    bool m_autoLogin = false;
    bool m_allowInsecureSsl = false;
    Lang m_lang = Lang::ZhCN;
    QString m_host;
    QString m_ip;
    QString m_nasip;
    int m_keepaliveMinSec = constants::KEEPALIVE_MIN_SEC;
    int m_keepaliveMaxSec = constants::KEEPALIVE_MAX_SEC;
    int m_offlineFailThreshold = constants::OFFLINE_FAIL_THRESHOLD;
};

} // namespace gdei

#endif // GDEI_CONFIG_STORE_HPP
