// gdei/json.hpp
// 极简 JSON 解析/序列化 / Minimal JSON parser & serializer.
// 只覆盖本项目需要的子集：对象、数组、字符串、数字、布尔、null。
// Only the subset this project needs; no third-party dependency.
#ifndef GDEI_JSON_HPP
#define GDEI_JSON_HPP

#include <map>
#include <string>
#include <vector>

namespace gdei {

struct JsonValue {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool boolean = false;
    double number = 0.0;
    std::string str;
    std::vector<JsonValue> arr;
    std::map<std::string, JsonValue> obj;

    bool isObject() const { return type == Object; }
    const JsonValue* find(const std::string& key) const;
    // 取字符串值；非字符串时返回空串。
    std::string asString() const;
};

// 解析成功返回 true；失败时 *err 带原因。
bool parseJson(const std::string& text, JsonValue& out, std::string* err = 0);

// 序列化为紧凑 JSON。
std::string dumpJson(const JsonValue& v);

} // namespace gdei

#endif // GDEI_JSON_HPP
