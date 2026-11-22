// src/platform/TrayController.hpp
// Windows 系统托盘桥接；认证动作仍由 AuthController 执行。
#ifndef GDEI_TRAY_CONTROLLER_HPP
#define GDEI_TRAY_CONTROLLER_HPP

#include <QObject>
#include <QPointer>
#include <Qt>

class QAction;
class QMenu;
class QQuickWindow;
class QSystemTrayIcon;

namespace gdei {

class AuthController;
class SessionController;
class I18n;

class TrayController : public QObject {
    Q_OBJECT

public:
    TrayController(QQuickWindow* window, SessionController* session,
                   AuthController* auth, I18n* i18n, QObject* parent = nullptr);
    ~TrayController() override;

private:
    void showWindow();
    void onWindowStateChanged(Qt::WindowState state);
    void updateTexts();
    void updateStatus();
    void updateActions();

    QPointer<QQuickWindow> m_window;
    SessionController* m_session = nullptr;
    AuthController* m_auth = nullptr;
    I18n* m_i18n = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_showAction = nullptr;
    QAction* m_loginAction = nullptr;
    QAction* m_logoutAction = nullptr;
    QAction* m_quitAction = nullptr;
};

} // namespace gdei

#endif // GDEI_TRAY_CONTROLLER_HPP
