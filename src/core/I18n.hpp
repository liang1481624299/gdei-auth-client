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
