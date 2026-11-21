// gui/qt/mainwindow.hpp
// Qt 主窗口 / Qt main window.
// 仅为桌面便利操作；核心能力在 core 库，CLI 与 GUI 共用。
// Thin desktop wrapper around the shared core library.
#ifndef GDEI_MAINWINDOW_HPP
#define GDEI_MAINWINDOW_HPP

#include "gdei/config.hpp"

#include <QMainWindow>
#include <QTimer>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0);

private slots:
    void onStatus();
    void onLogin();
    void onLogout();
    void onSave();
    void onToggleKeep();
    void onKeepTick();

private:
    void buildUi();
    void fillFromConfig();
    void readFieldsToConfig();
    void log(const QString& msg);

    gdei::Config cfg_;

    QLineEdit* userEdit = 0;
    QLineEdit* passEdit = 0;
    QLineEdit* hostEdit = 0;
    QLineEdit* ipEdit = 0;
    QLineEdit* nasipEdit = 0;
    QLineEdit* intervalEdit = 0;
    QPlainTextEdit* logView = 0;
    QPushButton* keepBtn = 0;
    QTimer* timer_ = 0;
    bool keeping_ = false;
};

#endif // GDEI_MAINWINDOW_HPP
