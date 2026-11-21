// gdei/config.cpp
#include "gdei/config.hpp"
#include "gdei/encoding.hpp"
#include "gdei/json.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace gdei {

Config Config::defaults() {
    Config c;
    c.host = "nac.gdei.edu.cn";
    c.ip = "210.38.64.39";
    c.scheme = "https";
    c.port = 443;
    c.nasip = "192.168.100.8";
    c.healthUrl = "http://nac.gdei.edu.cn:8080/mselfservice/snac/macList.xhtml";
    c.keepaliveIntervalSec = 60;
    return c;
}

std::string Config::defaultPath() {
    const char* env = std::getenv("GDEI_CONFIG");
    if (env && *env) return std::string(env);
    return "config.json";
}

bool Config::load(const std::string& path, std::string* err) {
    Config d = defaults();
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f) {
        // 文件不存在不算错误，沿用默认值。
        *this = d;
        return true;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string text = ensureUtf8(ss.str());

    JsonValue root;
    std::string jerr;
    if (!parseJson(text, root, &jerr) || !root.isObject()) {
        if (err) *err = "config parse error: " + jerr;
        *this = d;
        return false;
    }

    *this = d;
    if (const JsonValue* v = root.find("username")) username = v->asString();
    if (const JsonValue* v = root.find("password_ob")) passwordOb = v->asString();
    if (const JsonValue* v = root.find("host")) host = v->asString();
    if (const JsonValue* v = root.find("ip")) ip = v->asString();
    if (const JsonValue* v = root.find("scheme")) scheme = v->asString();
    if (const JsonValue* v = root.find("nasip")) nasip = v->asString();
    if (const JsonValue* v = root.find("healthUrl")) healthUrl = v->asString();
    if (const JsonValue* v = root.find("port")) { if (v->type == JsonValue::Number) port = static_cast<int>(v->number); }
    if (const JsonValue* v = root.find("keepaliveIntervalSec")) {
        if (v->type == JsonValue::Number) keepaliveIntervalSec = static_cast<int>(v->number);
    }
    if (port <= 0) port = (scheme == "https" ? 443 : 80);
    if (keepaliveIntervalSec <= 0) keepaliveIntervalSec = 60;
    return true;
}

bool Config::save(const std::string& path, std::string* err) const {
    JsonValue root;
    root.type = JsonValue::Object;
    root.obj["username"].type = JsonValue::String; root.obj["username"].str = username;
    root.obj["password_ob"].type = JsonValue::String; root.obj["password_ob"].str = passwordOb;
    root.obj["host"].type = JsonValue::String; root.obj["host"].str = host;
    root.obj["ip"].type = JsonValue::String; root.obj["ip"].str = ip;
    root.obj["scheme"].type = JsonValue::String; root.obj["scheme"].str = scheme;
    root.obj["nasip"].type = JsonValue::String; root.obj["nasip"].str = nasip;
    root.obj["healthUrl"].type = JsonValue::String; root.obj["healthUrl"].str = healthUrl;
    root.obj["port"].type = JsonValue::Number; root.obj["port"].number = port;
    root.obj["keepaliveIntervalSec"].type = JsonValue::Number;
    root.obj["keepaliveIntervalSec"].number = keepaliveIntervalSec;

    std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!f) { if (err) *err = "cannot write config: " + path; return false; }
    f << dumpJson(root);
    return true;
}

std::string Config::plainPassword() const {
    return deobfuscate(passwordOb);
}

void Config::setPlainPassword(const std::string& plain) {
    passwordOb = obfuscate(ensureUtf8(plain));
}

} // namespace gdei
