// src/core/I18n.cpp
#include "core/I18n.hpp"

namespace gdei {

I18n::I18n(QObject* parent) : QObject(parent) {
    buildDictionaries();
}

int I18n::lang() const { return static_cast<int>(m_lang); }

QVariantList I18n::languages() const {
    QVariantList out;
    const Lang order[] = {Lang::ZhCN, Lang::ZhTW, Lang::EnGB, Lang::Ja};
    for (Lang l : order) {
        QVariantMap m;
        m["value"] = static_cast<int>(l);
        m["name"] = displayName(l);
        out.append(m);
    }
    return out;
}

void I18n::setLang(int l) {
    Lang nl = static_cast<Lang>(l);
    if (m_lang == nl) return;
    m_lang = nl;
    emit langChanged();
}

QString I18n::displayName(Lang l) {
    switch (l) {
        case Lang::ZhTW: return QStringLiteral("繁體中文");
        case Lang::EnGB: return QStringLiteral("English");
        case Lang::Ja:   return QStringLiteral("日本語");
        case Lang::ZhCN:
        default:         return QStringLiteral("简体中文");
    }
}

QString I18n::text(const QString& key) const {
    const QMap<QString, QString>& cur = m_dict.value(m_lang);
    if (cur.contains(key)) return cur.value(key);
    const QMap<QString, QString>& en = m_dict.value(Lang::EnGB);
    if (en.contains(key)) return en.value(key);
    return key;
}

void I18n::buildDictionaries() {
    QMap<QString, QString> zhCN;
    zhCN["app.title"] = "GDEI 校园网认证客户端";
    zhCN["status.online"] = "在线";
    zhCN["status.offline"] = "未上线";
    zhCN["login.success"] = "认证成功，已上线";
    zhCN["login.failed"] = "认证失败";
    zhCN["logout.success"] = "已下线";
    zhCN["config.saved"] = "配置已保存";
    zhCN["config.missingCred"] = "未配置账号或密码";
    zhCN["keepalive.started"] = "保活已启动";
    zhCN["keepalive.stopped"] = "保活已停止";
    zhCN["keepalive.offlineReauth"] = "掉线，尝试重新认证";
    zhCN["btn.status"] = "状态";
    zhCN["btn.login"] = "上线";
    zhCN["btn.logout"] = "下线";
    zhCN["btn.save"] = "保存";
    zhCN["btn.keepStart"] = "启动保活";
    zhCN["btn.keepStop"] = "停止保活";
    zhCN["label.language"] = "语言";
    zhCN["label.username"] = "账号";
    zhCN["label.password"] = "密码";
    zhCN["label.host"] = "门户";
    zhCN["label.ip"] = "直连 IP";
    zhCN["label.nasip"] = "NAS / AC";
    zhCN["label.interval"] = "保活间隔(秒)";
    zhCN["action.remember"] = "记住密码";
    zhCN["action.autoLogin"] = "自动登录";
    zhCN["log.title"] = "日志";
    zhCN["log.export"] = "导出日志";
    zhCN["err.request"] = "请求异常";
    m_dict[Lang::ZhCN] = zhCN;

    QMap<QString, QString> zhTW;
    zhTW["app.title"] = "GDEI 校園網認證客戶端";
    zhTW["status.online"] = "線上";
    zhTW["status.offline"] = "未上線";
    zhTW["login.success"] = "認證成功，已上線";
    zhTW["login.failed"] = "認證失敗";
    zhTW["logout.success"] = "已下線";
    zhTW["config.saved"] = "設定已儲存";
    zhTW["config.missingCred"] = "未設定帳號或密碼";
    zhTW["keepalive.started"] = "保活已啟動";
    zhTW["keepalive.stopped"] = "保活已停止";
    zhTW["keepalive.offlineReauth"] = "斷線，嘗試重新認證";
    zhTW["btn.status"] = "狀態";
    zhTW["btn.login"] = "上線";
    zhTW["btn.logout"] = "下線";
    zhTW["btn.save"] = "儲存";
    zhTW["btn.keepStart"] = "啟動保活";
    zhTW["btn.keepStop"] = "停止保活";
    zhTW["label.language"] = "語言";
    zhTW["label.username"] = "帳號";
    zhTW["label.password"] = "密碼";
    zhTW["label.host"] = "入口";
    zhTW["label.ip"] = "直連 IP";
    zhTW["label.nasip"] = "NAS / AC";
    zhTW["label.interval"] = "保活間隔(秒)";
    zhTW["action.remember"] = "記住密碼";
    zhTW["action.autoLogin"] = "自動登入";
    zhTW["log.title"] = "日誌";
    zhTW["log.export"] = "匯出日誌";
    zhTW["err.request"] = "請求異常";
    m_dict[Lang::ZhTW] = zhTW;

    QMap<QString, QString> enGB;
    enGB["app.title"] = "GDEI Campus Network Auth Client";
    enGB["status.online"] = "Online";
    enGB["status.offline"] = "Offline";
    enGB["login.success"] = "Authenticated, online";
    enGB["login.failed"] = "Login failed";
    enGB["logout.success"] = "Logged out";
    enGB["config.saved"] = "Config saved";
    enGB["config.missingCred"] = "Username or password not set";
    enGB["keepalive.started"] = "Keep-alive started";
    enGB["keepalive.stopped"] = "Keep-alive stopped";
    enGB["keepalive.offlineReauth"] = "Offline, re-authenticating";
    enGB["btn.status"] = "Status";
    enGB["btn.login"] = "Login";
    enGB["btn.logout"] = "Logout";
    enGB["btn.save"] = "Save";
    enGB["btn.keepStart"] = "Start keep-alive";
    enGB["btn.keepStop"] = "Stop";
    enGB["label.language"] = "Language";
    enGB["label.username"] = "Username";
    enGB["label.password"] = "Password";
    enGB["label.host"] = "Host";
    enGB["label.ip"] = "Direct IP";
    enGB["label.nasip"] = "NAS / AC";
    enGB["label.interval"] = "Interval (s)";
    enGB["action.remember"] = "Remember password";
    enGB["action.autoLogin"] = "Auto login";
    enGB["log.title"] = "Log";
    enGB["log.export"] = "Export log";
    enGB["err.request"] = "Request error";
    m_dict[Lang::EnGB] = enGB;

    QMap<QString, QString> ja;
    ja["app.title"] = "GDEI キャンパス認証クライアント";
    ja["status.online"] = "オンライン";
    ja["status.offline"] = "オフライン";
    ja["login.success"] = "認証成功、オンライン";
    ja["login.failed"] = "認証失敗";
    ja["logout.success"] = "ログアウトしました";
    ja["config.saved"] = "設定を保存しました";
    ja["config.missingCred"] = "ユーザー名またはパスワードが未設定です";
    ja["keepalive.started"] = "キープアライブ開始";
    ja["keepalive.stopped"] = "キープアライブ停止";
    ja["keepalive.offlineReauth"] = "オフライン、再認証します";
    ja["btn.status"] = "状態";
    ja["btn.login"] = "ログイン";
    ja["btn.logout"] = "ログアウト";
    ja["btn.save"] = "保存";
    ja["btn.keepStart"] = "キープアライブ開始";
    ja["btn.keepStop"] = "停止";
    ja["label.language"] = "言語";
    ja["label.username"] = "ユーザー名";
    ja["label.password"] = "パスワード";
    ja["label.host"] = "ホスト";
    ja["label.ip"] = "直接 IP";
    ja["label.nasip"] = "NAS / AC";
    ja["label.interval"] = "間隔(秒)";
    ja["action.remember"] = "パスワードを保存";
    ja["action.autoLogin"] = "自動ログイン";
    ja["log.title"] = "ログ";
    ja["log.export"] = "ログ出力";
    ja["err.request"] = "リクエストエラー";
    m_dict[Lang::Ja] = ja;
}

} // namespace gdei
