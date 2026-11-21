// gdei/config.hpp
// 本地配置读写 / Local configuration (config.json).
// 口令以可逆混淆形式保存，不明文落盘。
// Password is stored obfuscated, never in plaintext.
#ifndef GDEI_CONFIG_HPP
#define GDEI_CONFIG_HPP

#include <string>

namespace gdei {

struct Config {
    std::string username;
    std::string passwordOb;      // 混淆后的口令 / obfuscated password
    std::string host;            // 门户域名 / portal host
    std::string ip;              // 备用直连 IP / fallback direct IP
    std::string scheme;          // http / https
    std::string nasip;           // NAS (AC) 地址
    std::string healthUrl;       // 门户可达性探测地址 / portal reachability probe
    int port = 443;
    int keepaliveIntervalSec = 60; // 保活轮询间隔 / keep-alive interval

    static Config defaults();

    // 配置文件路径：--config > 环境变量 GDEI_CONFIG > 当前目录 config.json
    static std::string defaultPath();

    bool load(const std::string& path, std::string* err = 0);
    bool save(const std::string& path, std::string* err = 0) const;

    std::string plainPassword() const;        // 还原明文口令 / deobfuscate
    void setPlainPassword(const std::string& plain);
};

} // namespace gdei

#endif // GDEI_CONFIG_HPP
