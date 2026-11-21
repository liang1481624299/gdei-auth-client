// src/core/ConfigStore.hpp
// 配置持久化 / Persistent settings via QSettings.
// 口令仅在“记住密码”开启时混淆保存，不明文落盘。
// Password is obfuscated on disk only when "remember password" is on.
#ifndef GDEI_CONFIG_STORE_HPP
#define GDEI_CONFIG_STORE_HPP

#include "core/Types.hpp"

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

    void setUsername(const QString& v);
    void setPassword(const QString& v);
    void setRememberPassword(bool v);
    void setAutoLogin(bool v);
    void setLang(int v);
    void setHost(const QString& v);
    void setIp(const QString& v);
    void setNasip(const QString& v);

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

private:
    QString m_username;
    QString m_password;        // 内存中的明文，不落盘 / plaintext in memory only
    QString m_passwordOb;      // 磁盘上的混淆值 / obfuscated value on disk
    bool m_rememberPassword = false;
    bool m_autoLogin = false;
    Lang m_lang = Lang::ZhCN;
    QString m_host;
    QString m_ip;
    QString m_nasip;
};

} // namespace gdei

#endif // GDEI_CONFIG_STORE_HPP
