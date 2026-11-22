// src/core/I18n.hpp
// 多语言 / Internationalization.
// 简体中文 / 繁體中文 / English (UK) / 日本語，暴露给 QML 切换。
#ifndef GDEI_I18N_HPP
#define GDEI_I18N_HPP

#include "core/Types.hpp"

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>

namespace gdei {

class I18n : public QObject {
    Q_OBJECT
    Q_PROPERTY(int lang READ lang WRITE setLang NOTIFY langChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)

public:
    explicit I18n(QObject* parent = nullptr);

    int lang() const;
    QVariantList languages() const;

    Q_INVOKABLE void setLang(int l);
    // 取译文；缺键回退英文，再回退键名。/ Translate key with EN fallback.
    Q_INVOKABLE QString text(const QString& key) const;
    // 键是否存在于当前或英文词典。/ Whether a translation key is defined.
    Q_INVOKABLE bool hasKey(const QString& key) const;
    // 统一消息本地化：payload 形如 "key|arg1|arg2"，先翻译 key 再按序 arg()；
    // 非已注册 key 则原样返回（兼容服务端原文与历史双语串）。
    // Unified message localization for key[|arg] payloads; unknown payloads pass through.
    Q_INVOKABLE QString localize(const QString& payload) const;

    static QString displayName(Lang l);

signals:
    void langChanged();

private:
    Lang m_lang = Lang::ZhCN;
    QMap<Lang, QMap<QString, QString>> m_dict;

    void buildDictionaries();
};

} // namespace gdei

#endif // GDEI_I18N_HPP
