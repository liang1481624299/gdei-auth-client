// src/main.cpp
// 程序入口 / Application entry point.
// 装配核心对象并注入 QML 根上下文；业务逻辑全部在 C++，QML 只做交互。
// Wire up core objects and inject them into the QML root context.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "app/AppController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/LogController.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

using namespace gdei;

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("gdei-connect"));
    app.setOrganizationName(QStringLiteral("GDEI"));

    // 持久化配置 / Persistent settings.
    ConfigStore config;
    config.load();

    Logger logger;
    I18n i18n;
    i18n.setLang(config.lang());
    QObject::connect(&i18n, &I18n::langChanged, &config, [&] {
        config.setLang(i18n.lang());
    });

    AuthController auth(&config, &logger, &i18n);
    LogController logs(&logger);
    DeviceController devices(&config, &logger);
    AppController controller(&config, &logger, &i18n, &auth, &logs, &devices);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);

    // 按模块 URI + 组件名加载入口，避免手写 qrc 路径受资源前缀 / 目录层级影响。
    // Load the entry by module URI and element name instead of a hand-written qrc path,
    // so the entry is not sensitive to the resource prefix or directory layout.
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QGuiApplication::exit(-1); });
    engine.loadFromModule(QStringLiteral("GdeiConnect"), QStringLiteral("Main"));

    // 自动登录 / Auto login when enabled and credentials are present.
    if (config.autoLogin() && !config.username().isEmpty() &&
        !config.password().isEmpty()) {
        auth.login();
    }

    QObject::connect(&app, &QGuiApplication::aboutToQuit, &config, &ConfigStore::save);
    return app.exec();
}
