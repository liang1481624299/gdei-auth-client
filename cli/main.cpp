// cli/main.cpp
// 命令行前端 / Command-line interface (headless core).
// 图形界面只是桌面便利操作；无显示器的设备直接用本程序。
// GUI is just a desktop convenience; headless devices use this CLI.
//
// 用法 / Usage:
//   gdei-auth status                 查看是否在线 / check online status
//   gdei-auth login                  上线 / authenticate
//   gdei-auth logout                 下线 / log out
//   gdei-auth keepalive              后台保活，掉线自动重登 / auto keep-alive loop
//   gdei-auth configure --user U --pass P [--interval 60]   保存配置 / save config
//   gdei-auth --help
//
// 通用参数 / Common options:
//   --config <path>                  指定配置文件 / config file path
//   --user <name>  --pass <pwd>      临时覆盖账号口令 / override for one run

#include "gdei/config.hpp"
#include "gdei/encoding.hpp"
#include "gdei/keepalive.hpp"
#include "gdei/portal_client.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

#if defined(_WIN32)
std::vector<std::string> getArgs() {
    std::vector<std::string> a;
    for (int i = 0; i < __argc; ++i) a.push_back(gdei::wideToUtf8(__wargv[i]));
    return a;
}
#else
std::vector<std::string> getArgs(int argc, char** argv) {
    std::vector<std::string> a;
    for (int i = 0; i < argc; ++i) a.push_back(argv[i]);
    return a;
}
#endif

gdei::KeepAlive* g_keepAlive = 0;

void onStopSignal(int) {
    if (g_keepAlive) g_keepAlive->stop();
}

void printHelp() {
    std::cout <<
        "GDEI 校园网认证客户端 / GDEI campus network auth client\n"
        "\n"
        "Commands:\n"
        "  status                        Check whether online\n"
        "  login                         Authenticate / come online\n"
        "  logout                        Log out / go offline\n"
        "  keepalive                     Auto keep-alive loop (re-login if offline)\n"
        "  configure --user U --pass P   Save username/password to config\n"
        "             [--interval 60]    Keep-alive interval seconds\n"
        "\n"
        "Options:\n"
        "  --config <path>               Config file (default: ./config.json)\n"
        "  --user <name> --pass <pwd>    Override credentials for this run\n"
        "  --help                        Show this help\n";
}

struct Options {
    std::string command = "status";
    std::string configPath;
    std::string user;
    std::string pass;
    int interval = -1;
};

// 解析参数；返回 false 表示参数有误。
bool parseArgs(const std::vector<std::string>& args, Options& o) {
    bool positionalDone = false;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--help" || a == "-h") { o.command = "help"; }
        else if (a == "--config" && i + 1 < args.size()) o.configPath = args[++i];
        else if (a == "--user" && i + 1 < args.size()) o.user = args[++i];
        else if (a == "--pass" && i + 1 < args.size()) o.pass = args[++i];
        else if (a == "--interval" && i + 1 < args.size()) o.interval = std::atoi(args[++i].c_str());
        else if (!a.empty() && a[0] == '-') { std::cerr << "unknown option: " << a << "\n"; return false; }
        else if (!positionalDone) { o.command = a; positionalDone = true; } // 第一个位置参数作为命令
        else { std::cerr << "unexpected argument: " << a << "\n"; return false; }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
#if defined(_WIN32)
    std::vector<std::string> args = getArgs();
#else
    std::vector<std::string> args = getArgs(argc, argv);
#endif

    Options o;
    if (!parseArgs(args, o)) { printHelp(); return 2; }
    if (o.command == "help") { printHelp(); return 0; }

    gdei::Config cfg;
    std::string cfgPath = o.configPath.empty() ? gdei::Config::defaultPath() : o.configPath;
    std::string err;
    if (!cfg.load(cfgPath, &err)) {
        std::cerr << "配置读取警告 / config warning: " << err << "\n";
    }
    if (!o.user.empty()) cfg.username = o.user;
    if (!o.pass.empty()) cfg.setPlainPassword(o.pass);
    if (o.interval > 0) cfg.keepaliveIntervalSec = o.interval;

    if (o.command == "configure") {
        if (cfg.username.empty()) {
            std::cerr << "请用 --user 指定账号 / use --user\n";
            return 2;
        }
        if (!cfg.save(cfgPath, &err)) {
            std::cerr << "保存失败 / save failed: " << err << "\n";
            return 1;
        }
        std::cout << "配置已保存 / config saved: " << cfgPath << "\n";
        return 0;
    }

    if (o.command == "status") {
        gdei::PortalClient c(cfg);
        std::string detail;
        bool portal = c.probePortal(&detail);
        bool online = c.probeInternet(6);
        std::cout << (online ? "在线 / online" : "未上线 / offline");
        std::cout << "  (门户 " << detail << ", reachable=" << (portal ? "yes" : "no") << ")\n";
        return online ? 0 : 1;
    }

    if (o.command == "login") {
        gdei::PortalClient c(cfg);
        gdei::Outcome r = c.login();
        std::cout << r.message << "\n";
        return r.ok ? 0 : 1;
    }

    if (o.command == "logout") {
        gdei::PortalClient c(cfg);
        gdei::Outcome r = c.logout();
        std::cout << r.message << "\n";
        return r.ok ? 0 : 1;
    }

    if (o.command == "keepalive") {
        std::signal(SIGINT, onStopSignal);
        std::signal(SIGTERM, onStopSignal);
        gdei::KeepAlive ka(cfg, [](const std::string& m) { std::cout << m << "\n"; });
        g_keepAlive = &ka;
        ka.loop();
        std::cout << "已停止保活 / keep-alive stopped\n";
        return 0;
    }

    printHelp();
    return 2;
}
