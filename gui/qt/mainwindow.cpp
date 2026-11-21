// gui/qt/mainwindow.cpp
#include "mainwindow.hpp"

#include "gdei/keepalive.hpp"
#include "gdei/portal_client.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QString u8(const std::string& s) { return QString::fromUtf8(s.c_str()); }
std::string u8(const QString& s) { return s.toUtf8().constData(); }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    std::string err;
    cfg_.load(gdei::Config::defaultPath(), &err);
    buildUi();
    fillFromConfig();
    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(onKeepTick()));
}

void MainWindow::buildUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QFormLayout* form = new QFormLayout();
    userEdit = new QLineEdit();
    passEdit = new QLineEdit();
    passEdit->setEchoMode(QLineEdit::Password);
    hostEdit = new QLineEdit();
    ipEdit = new QLineEdit();
    nasipEdit = new QLineEdit();
    intervalEdit = new QLineEdit();
    intervalEdit->setMaximumWidth(80);

    form->addRow("账号 / Username", userEdit);
    form->addRow("密码 / Password", passEdit);
    form->addRow("门户 / Host", hostEdit);
    form->addRow("直连 IP / IP", ipEdit);
    form->addRow("NAS / AC", nasipEdit);
    form->addRow("保活间隔(秒) / Interval", intervalEdit);

    QPushButton* statusBtn = new QPushButton("状态 / Status");
    QPushButton* loginBtn = new QPushButton("上线 / Login");
    QPushButton* logoutBtn = new QPushButton("下线 / Logout");
    QPushButton* saveBtn = new QPushButton("保存 / Save");
    keepBtn = new QPushButton("启动保活 / Start keep-alive");

    QHBoxLayout* btns = new QHBoxLayout();
    btns->addWidget(statusBtn);
    btns->addWidget(loginBtn);
    btns->addWidget(logoutBtn);
    btns->addWidget(saveBtn);
    btns->addWidget(keepBtn);

    logView = new QPlainTextEdit();
    logView->setReadOnly(true);

    QVBoxLayout* root = new QVBoxLayout(central);
    root->addLayout(form);
    root->addLayout(btns);
    root->addWidget(logView);

    connect(statusBtn, SIGNAL(clicked()), this, SLOT(onStatus()));
    connect(loginBtn, SIGNAL(clicked()), this, SLOT(onLogin()));
    connect(logoutBtn, SIGNAL(clicked()), this, SLOT(onLogout()));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(onSave()));
    connect(keepBtn, SIGNAL(clicked()), this, SLOT(onToggleKeep()));

    setWindowTitle("GDEI 校园网认证 / Campus Auth");
    resize(480, 420);
}

void MainWindow::fillFromConfig() {
    userEdit->setText(u8(cfg_.username));
    passEdit->clear(); // 口令不明文回显 / do not echo obfuscated password
    hostEdit->setText(u8(cfg_.host));
    ipEdit->setText(u8(cfg_.ip));
    nasipEdit->setText(u8(cfg_.nasip));
    intervalEdit->setText(QString::number(cfg_.keepaliveIntervalSec));
}

void MainWindow::readFieldsToConfig() {
    cfg_.username = u8(userEdit->text());
    if (!passEdit->text().isEmpty()) cfg_.setPlainPassword(u8(passEdit->text()));
    cfg_.host = u8(hostEdit->text());
    cfg_.ip = u8(ipEdit->text());
    cfg_.nasip = u8(nasipEdit->text());
    bool ok = false;
    int iv = intervalEdit->text().toInt(&ok);
    if (ok && iv > 0) cfg_.keepaliveIntervalSec = iv;
}

void MainWindow::log(const QString& msg) {
    logView->appendPlainText(msg);
}

void MainWindow::onStatus() {
    gdei::PortalClient c(cfg_);
    std::string detail;
    bool portal = c.probePortal(&detail);
    bool online = c.probeInternet(6);
    log(online ? "在线 / online" : "未上线 / offline");
    log(QString("门户 / portal: %1 (reachable=%2)")
        .arg(u8(detail)).arg(portal ? "yes" : "no"));
}

void MainWindow::onLogin() {
    readFieldsToConfig();
    gdei::PortalClient c(cfg_);
    gdei::Outcome r = c.login();
    log(u8(r.message));
}

void MainWindow::onLogout() {
    readFieldsToConfig();
    gdei::PortalClient c(cfg_);
    gdei::Outcome r = c.logout();
    log(u8(r.message));
}

void MainWindow::onSave() {
    readFieldsToConfig();
    std::string err;
    if (cfg_.save(gdei::Config::defaultPath(), &err)) {
        log("配置已保存 / config saved");
    } else {
        log("保存失败 / save failed: " + u8(err));
    }
}

void MainWindow::onToggleKeep() {
    if (!keeping_) {
        readFieldsToConfig();
        timer_->start(cfg_.keepaliveIntervalSec * 1000);
        keeping_ = true;
        keepBtn->setText("停止保活 / Stop");
        log("保活已启动 / keep-alive started");
    } else {
        timer_->stop();
        keeping_ = false;
        keepBtn->setText("启动保活 / Start keep-alive");
        log("保活已停止 / keep-alive stopped");
    }
}

void MainWindow::onKeepTick() {
    gdei::KeepAlive ka(cfg_, [this](const std::string& m) { log(u8(m)); });
    ka.runOnce();
}
