// src/core/Types.hpp
// 公共枚举 / Shared enums.
// 注册到 Qt 元系统，供 QML 信号槽使用 / Registered for QML signals & slots.
#ifndef GDEI_TYPES_HPP
#define GDEI_TYPES_HPP

#include <QObject>

namespace gdei {

Q_NAMESPACE

// 日志级别 / Log level.
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};
Q_ENUM_NS(LogLevel)

// 认证状态 / Authentication state.
enum class AuthState {
    Unknown,
    Offline,
    Authenticating,
    Online,
    Error
};
Q_ENUM_NS(AuthState)

// 界面语言 / UI language.
enum class Lang {
    ZhCN,   // 简体中文
    ZhTW,   // 繁體中文
    EnGB,   // English (UK)
    Ja      // 日本語
};
Q_ENUM_NS(Lang)

} // namespace gdei

#endif // GDEI_TYPES_HPP
