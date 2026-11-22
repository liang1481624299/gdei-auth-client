// src/platform/TrayController.cpp
#include "platform/TrayController.hpp"

#include "app/AuthController.hpp"
#include "app/SessionController.hpp"
#include "core/I18n.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QMenu>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QTimer>

namespace gdei {

TrayController::TrayController(QQuickWindow* window, SessionController* session,
                               AuthController* auth, I18n* i18n, QObject* parent)
    : QObject(parent), m_window(window), m_session(session), m_auth(auth), m_i18n(i18n) {
    if (!m_window || !m_session || !m_auth || !m_i18n || !QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    QApplication::setQuitOnLastWindowClosed(false);
    m_tray = new QSystemTrayIcon(this);
    // QMenu 继承 QWidget，parent 形参只接受 QWidget*；TrayController 是 QObject，
    // 不能以 this 作父对象，故无 parent，析构中手动释放。
    m_menu = new QMenu();
    m_showAction = m_menu->addAction(QString());
    m_loginAction = m_menu->addAction(QString());
    m_logoutAction = m_menu->addAction(QString());
    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(QString());
    m_tray->setContextMenu(m_menu);

    connect(m_showAction, &QAction::triggered, this, &TrayController::showWindow);
    connect(m_loginAction, &QAction::triggered, m_session, &SessionController::login);
    connect(m_logoutAction, &QAction::triggered, m_session, &SessionController::logout);
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger ||
                    reason == QSystemTrayIcon::DoubleClick) {
                    showWindow();
                }
            });
    connect(m_window, &QWindow::windowStateChanged,
            this, &TrayController::onWindowStateChanged);
    connect(m_auth, &AuthController::onlineChanged, this, &TrayController::updateStatus);
    connect(m_auth, &AuthController::stateChanged, this, &TrayController::updateActions);
    connect(m_session, &SessionController::busyChanged, this, &TrayController::updateActions);
    connect(m_session, &SessionController::loggedInChanged, this, &TrayController::updateActions);
    connect(m_i18n, &I18n::langChanged, this, &TrayController::updateTexts);

    updateTexts();
    updateActions();
    m_tray->show();
}

TrayController::~TrayController() {
    if (m_tray) m_tray->hide();
    // m_menu 无 QObject 父对象，需手动释放；先从托盘摘下再删。
    if (m_tray) m_tray->setContextMenu(nullptr);
    delete m_menu;
    m_menu = nullptr;
}

void TrayController::showWindow() {
    if (!m_window) return;
    m_window->setWindowState(Qt::WindowNoState);
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}

void TrayController::onWindowStateChanged(Qt::WindowState state) {
    if (state != Qt::WindowMinimized || !m_tray || !m_window) return;
    // 等原生最小化状态提交后再隐藏，避免 Windows 动画与 QML 状态竞争。
    QTimer::singleShot(0, this, [this] {
        if (m_window && m_window->windowState() == Qt::WindowMinimized) {
            m_window->hide();
        }
    });
}

void TrayController::updateTexts() {
    if (!m_tray) return;
    m_showAction->setText(m_i18n->text(QStringLiteral("tray.show")));
    m_loginAction->setText(m_i18n->text(QStringLiteral("btn.login")));
    m_logoutAction->setText(m_i18n->text(QStringLiteral("btn.logout")));
    m_quitAction->setText(m_i18n->text(QStringLiteral("tray.quit")));
    updateStatus();
}

void TrayController::updateStatus() {
    if (!m_tray || !m_auth || !m_i18n) return;
    const bool online = m_auth->online();
    const QString iconPath = online
        ? QStringLiteral(":/GdeiConnect/qml/icons/trayOnline.svg")
        : QStringLiteral(":/GdeiConnect/qml/icons/trayOffline.svg");
    const QString statusKey = online ? QStringLiteral("status.online")
                                     : QStringLiteral("status.offline");
    m_tray->setIcon(QIcon(iconPath));
    m_tray->setToolTip(m_i18n->text(QStringLiteral("app.title")) +
                       QStringLiteral(" — ") + m_i18n->text(statusKey));
}

void TrayController::updateActions() {
    if (!m_session) return;
    const bool busy = m_session->busy();
    if (m_loginAction) m_loginAction->setEnabled(!busy);
    if (m_logoutAction) m_logoutAction->setEnabled(m_session->loggedIn() && !busy);
}

} // namespace gdei
