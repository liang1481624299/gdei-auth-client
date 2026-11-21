// src/main.cpp
// 程序入口 / Application entry point.
// 装配核心对象并注入 QML 根上下文；业务逻辑全部在 C++，QML 只做交互。
// Wire up core objects and inject them into the QML root context.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QDateTime>
#include <QMessageLogContext>

#include "app/AppController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/LogController.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

using namespace gdei;

// TEMP diagnostic: append stage trace to a plain TEMP file (no QStandardPaths/Org dependency).
// 临时诊断：把阶段轨迹写到 TEMP 下的纯文件，避免早期路径解析问题。
static void gcStage(const QString& msg) {
    QFile file(QDir::tempPath() + QStringLiteral("/gdei-stage.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&file);
        ts << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << " " << msg << "\n";
    }
}

static void gcDiagnosticHandler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    const char* level = "?";
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtWarningMsg:  level = "WARN";  break;
    case QtCriticalMsg: level = "CRIT";  break;
    case QtFatalMsg:    level = "FATAL"; break;
    default: break;
    }
    gcStage(QStringLiteral("[%1] %2").arg(QString::fromLatin1(level), msg));
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(gcDiagnosticHandler);
    gcStage(QStringLiteral("=== main entered ==="));
    QGuiApplication app(argc, argv);
    gcStage(QStringLiteral("app created"));
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
    gcStage(QStringLiteral("controller ready"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);

    // 入口 QML 位于编译期资源中；路径由 qt_add_qml_module 的资源前缀与文件相对位置决定。
    // The entry QML lives in the compiled resource; the path follows the module's
    // resource prefix plus the file's relative position. load(QUrl) is used to stay
    // compatible with Qt 6.4 (Debian bookworm), which lacks loadFromModule (6.5+).
    const QUrl url(QStringLiteral("qrc:/GdeiConnect/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { gcStage(QStringLiteral("!!! objectCreationFailed")); QGuiApplication::exit(-1); });
    gcStage(QStringLiteral("loading ") + url.toString());
    engine.load(url);
    gcStage(QStringLiteral("loaded, rootObjects=%1").arg(engine.rootObjects().size()));

    // 自动登录 / Auto login when enabled and credentials are present.
    if (config.autoLogin() && !config.username().isEmpty() &&
        !config.password().isEmpty()) {
        auth.login();
    }

    QObject::connect(&app, &QGuiApplication::aboutToQuit, &config, &ConfigStore::save);
    return app.exec();
}
