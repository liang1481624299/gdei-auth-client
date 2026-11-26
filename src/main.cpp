// src/main.cpp
// 程序入口 / Application entry point.
// 装配核心对象并注入 QML 根上下文；业务逻辑全部在 C++，QML 只做交互。
// Wire up core objects and inject them into the QML root context.
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <QApplication>
#include <QQuickWindow>
#else
#include <QGuiApplication>
#endif
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QDateTime>
#include <QMessageLogContext>
#include <QStringConverter>
#include <QIcon>
#include <QFontDatabase>
#include <QFont>
#include <QStringList>

#include "app/AppController.hpp"
#include "app/AccountController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/EventBridge.hpp"
#include "app/EventBus.hpp"
#include "app/LocalDeviceController.hpp"
#include "app/LogController.hpp"
#include "app/NetworkInfoController.hpp"
#include "app/DiagnosticsController.hpp"
#include "app/SessionController.hpp"
#include "core/CertificateStore.hpp"
#include "core/ConfigStore.hpp"
#include "core/PortalClient.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"
#ifdef Q_OS_WIN
#include "platform/TrayController.hpp"
#endif

using namespace gdei;

#ifdef Q_OS_WIN
using GdeiApplication = QApplication;
#else
using GdeiApplication = QGuiApplication;
#endif

// TEMP diagnostic: append stage trace to a plain TEMP file (no QStandardPaths/Org dependency).
// 临时诊断：把阶段轨迹写到 TEMP 下的纯文件，避免早期路径解析问题。
static void gcStage(const QString& msg) {
    QFile file(QDir::tempPath() + QStringLiteral("/gdei-stage.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&file);
        ts.setEncoding(QStringConverter::Utf8);
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

// 内置字体：从编译进 qrc 的字体文件注册到 QFontDatabase，保证无外部字体依赖时
// 中文 / 英文 / 日语 / 繁体中文都能正确渲染。资源前缀与 qt_add_qml_module 一致。
// Bundled fonts: register qrc fonts into QFontDatabase so each CJK script and
// Latin text render correctly even when the system lacks matching fonts.
static void gcRegisterBundledFonts() {
    // 资源路径用 ":/..." 形式；Qt 字体加载对 qrc:/ 前缀解析不可靠。
    // Use the ":/..." form; the qrc:/ prefix is not reliably parsed by font loading.
    const QStringList fonts = {
        QStringLiteral(":/GdeiConnect/qml/fonts/SimSun.ttc"),
        QStringLiteral(":/GdeiConnect/qml/fonts/SegoeUI.ttf"),
        QStringLiteral(":/GdeiConnect/qml/fonts/SegoeUI-Bold.ttf"),
        QStringLiteral(":/GdeiConnect/qml/fonts/Meiryo.ttc"),
        QStringLiteral(":/GdeiConnect/qml/fonts/MicrosoftJhengHei.ttc")
    };
    for (const QString& path : fonts) {
        if (!QFile::exists(path)) {
            gcStage(QStringLiteral("[font] resource missing %1").arg(path));
            continue;
        }
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0) {
            gcStage(QStringLiteral("[font] failed to load %1").arg(path));
            continue;
        }
        const QStringList fams = QFontDatabase::applicationFontFamilies(id);
        gcStage(QStringLiteral("[font] loaded %1 -> %2")
                    .arg(path, fams.join(QStringLiteral(", "))));
    }
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(gcDiagnosticHandler);
    gcStage(QStringLiteral("=== main entered ==="));
    GdeiApplication app(argc, argv);
    gcStage(QStringLiteral("app created"));
    app.setApplicationName(QStringLiteral("gdei-connect"));
    app.setOrganizationName(QStringLiteral("GDEI"));
    app.setWindowIcon(QIcon(QStringLiteral(":/GdeiConnect/qml/icons/app.png")));

    // 先注册内置字体，再设置全局字体与回退链；必须在 QML 加载前完成。
    // Register bundled fonts before setting the global font and fallback chain.
    gcRegisterBundledFonts();
    {
        // 主字体：Segoe UI 承载拉丁/英文；中文由回退链中的宋体补齐。
        // Primary family carries Latin; CJK glyphs come from the fallback chain.
        QFont baseFont(QStringLiteral("Segoe UI"));
        baseFont.setPixelSize(13);
        app.setFont(baseFont);

        // 应用级字体回退：按书写系统（中文 / 日语）依次兜底，内置字体优先，
        // 其次同语言系统字体，最后通用 sans-serif。繁简共用 Han 脚本，
        // 内置宋体覆盖简体、Microsoft JhengHei 覆盖繁体，缺失字形再回退系统字体。
        // Application-level fallback per script: bundled first, then system CJK,
        // then generic sans-serif.
        // 注意：setApplicationFallbackFontFamilies 自 Qt 6.8 起才提供；
        // 6.5/6.4 等旧版 Qt 无此 API，跳过显式回退链，交由系统默认字体合并兜底。
        // This API exists since Qt 6.8; on older Qt the system font fallback handles merging.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        const QStringList han = {
            QStringLiteral("SimSun"), QStringLiteral("Microsoft JhengHei"),
            QStringLiteral("Microsoft JhengHei UI"), QStringLiteral("Microsoft YaHei"),
            QStringLiteral("PingFang SC"), QStringLiteral("PingFang TC"),
            QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Noto Sans CJK TC"),
            QStringLiteral("sans-serif")};
        const QStringList ja = {
            QStringLiteral("MS UI Gothic"), QStringLiteral("MS Gothic"),
            QStringLiteral("Yu Gothic"), QStringLiteral("Noto Sans CJK JP"),
            QStringLiteral("sans-serif")};
        const QStringList latin = {
            QStringLiteral("Segoe UI"), QStringLiteral("sans-serif")};
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Common, latin);
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han, han);
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Katakana, ja);
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Hiragana, ja);
#endif
    }

    // 强制 Basic 样式：本项目深度自定义了控件的 background / contentItem / indicator，
    // 只有非原生样式（Basic/Fusion/Material）才允许这些定制；原生样式会忽略定制，
    // 导致全平台（Windows/macOS/Linux/Android）UI 不按设计渲染。在 engine.load 前设置。
    QQuickStyle::setStyle(QStringLiteral("Basic"));

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
    DeviceController portalDevices(&config, &logger, &i18n);
    AccountController account(&config, &logger, &i18n);
    SessionController session(&auth, &config, &i18n);
    LocalDeviceController localDevices(&config, &auth, &i18n);
    NetworkInfoController network(&config);
    DiagnosticsController diagnostics(&config);
    // 用户导入的 CA 信任锚：只追加信任，不放宽证书校验。
    CertificateStore certificates;
    if (auth.portal()) {
        auth.portal()->setCaCertificates(certificates.trustedCertificates());
    }
    QObject::connect(&certificates, &CertificateStore::changed, &certificates, [&] {
        if (auth.portal()) {
            auth.portal()->setCaCertificates(certificates.trustedCertificates());
        }
    });
    EventBus events;
    EventBridge eventBridge(&events, &auth, &session, &localDevices,
                            &portalDevices, &network, &config,
                            auth.keepAlive(), auth.portal());
    AppController controller(&config, &logger, &i18n, &auth, &logs, &session,
                             &localDevices, &network, &diagnostics, &events,
                             &portalDevices, &account,
                             &certificates);
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

#ifdef Q_OS_WIN
    // Windows 托盘只转发窗口与认证指令，不持有网络或配置业务逻辑。
    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0))) {
        new TrayController(window, &session, &auth, &i18n, &app);
    }
#endif

    // 启动先做一次只读连通探测标定初始在线态（不提交账号口令），避免未开自动登录时
    // 一打开就停留在默认离线态。探测结果由 onProbeReply 聚合外网可达信号。
    // Seed the initial state with a read-only reachability probe (no credentials), so the
    // UI does not start stuck offline when auto-login is disabled.
    auth.refreshStatus();

    // 自动登录 / Auto login when enabled and credentials are present.
    if (config.autoLogin() && !config.username().isEmpty() &&
        !config.password().isEmpty()) {
        session.loginAutomatically();
    }

    QObject::connect(&app, &QGuiApplication::aboutToQuit, &config, &ConfigStore::save);
    return app.exec();
}
