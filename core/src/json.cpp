// gdei/json.cpp
#include "gdei/json.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gdei {

const JsonValue* JsonValue::find(const std::string& key) const {
    std::map<std::string, JsonValue>::const_iterator it = obj.find(key);
    return it == obj.end() ? 0 : &it->second;
}

std::string JsonValue::asString() const {
    if (type == String) return str;
    if (type == Number) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%g", number);
        return std::string(buf);
    }
    if (type == Bool) return boolean ? "true" : "false";
    return std::string();
}

namespace {

struct Parser {
    const std::string& s;
    size_t i;
    std::string* err;

    Parser(const std::string& text, std::string* e) : s(text), i(0), err(e) {}

    void skipWs() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' ||
                                s[i] == '\n' || s[i] == '\r')) ++i;
    }
    bool fail(const std::string& msg) {
        if (err) *err = msg + " @ " + std::to_string(i);
        return false;
    }
    bool parseValue(JsonValue& v);

    void appendUtf8(unsigned cp, std::string& out) {
        if (cp < 0x80) out.push_back(static_cast<char>(cp));
        else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    int hex4() {
        if (i + 4 > s.size()) return -1;
        int cp = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++];
            int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else return -1;
            cp = (cp << 4) | d;
        }
        return cp;
    }

    bool parseString(std::string& out) {
        if (s[i] != '"') return fail("expected string");
        ++i;
        while (i < s.size() && s[i] != '"') {
            char c = s[i++];
            if (c != '\\') { out.push_back(c); continue; }
            if (i >= s.size()) return fail("bad escape");
            char e = s[i++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    int cp = hex4();
                    if (cp < 0) return fail("bad \\u");
                    appendUtf8(static_cast<unsigned>(cp), out);
                    break;
                }
                default: out.push_back(e); break;
            }
        }
        if (i >= s.size()) return fail("unterminated string");
        ++i;
        return true;
    }

    bool parseNumber(JsonValue& v) {
        size_t start = i;
        while (i < s.size() && (std::strchr("-+0123456789.eE", s[i]))) ++i;
        std::string tok = s.substr(start, i - start);
        v.type = JsonValue::Number;
        v.number = std::strtod(tok.c_str(), 0);
        v.str = tok;
        return true;
    }

    bool parseObject(JsonValue& v) {
        v.type = JsonValue::Object;
        ++i; skipWs();
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        while (i < s.size()) {
            skipWs();
            std::string key;
            if (!parseString(key)) return false;
            skipWs();
            if (i >= s.size() || s[i] != ':') return fail("expected ':'");
            ++i; skipWs();
            JsonValue child;
            if (!parseValue(child)) return false;
            v.obj[key] = child;
            skipWs();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == '}') { ++i; return true; }
            return fail("expected ',' or '}'");
        }
        return fail("unterminated object");
    }

    bool parseArray(JsonValue& v) {
        v.type = JsonValue::Array;
        ++i; skipWs();
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        while (i < s.size()) {
            JsonValue child;
            if (!parseValue(child)) return false;
            v.arr.push_back(child);
            skipWs();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == ']') { ++i; return true; }
            return fail("expected ',' or ']'");
        }
        return fail("unterminated array");
    }
};

bool Parser::parseValue(JsonValue& v) {
    skipWs();
    if (i >= s.size()) return fail("empty value");
    char c = s[i];
    if (c == '{') return parseObject(v);
    if (c == '[') return parseArray(v);
    if (c == '"') { v.type = JsonValue::String; return parseString(v.str); }
    if (c == 't') {
        if (s.compare(i, 4, "true") == 0) { v.type = JsonValue::Bool; v.boolean = true; i += 4; return true; }
        return fail("bad literal");
    }
    if (c == 'f') {
        if (s.compare(i, 5, "false") == 0) { v.type = JsonValue::Bool; v.boolean = false; i += 5; return true; }
        return fail("bad literal");
    }
    if (c == 'n') {
        if (s.compare(i, 4, "null") == 0) { v.type = JsonValue::Null; i += 4; return true; }
        return fail("bad literal");
    }
    return parseNumber(v);
}

std::string escapeString(const std::string& in) {
    std::string out;
    out.push_back('"');
    for (unsigned char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    out.push_back('"');
    return out;
}

} // namespace

bool parseJson(const std::string& text, JsonValue& out, std::string* err) {
    Parser p(text, err);
    bool ok = p.parseValue(out);
    return ok;
}

std::string dumpJson(const JsonValue& v) {
    switch (v.type) {
        case JsonValue::Null: return "null";
        case JsonValue::Bool: return v.boolean ? "true" : "false";
        case JsonValue::Number: {
            if (!v.str.empty()) return v.str;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%g", v.number);
            return std::string(buf);
        }
        case JsonValue::String: return escapeString(v.str);
        case JsonValue::Array: {
            std::string out = "[";
            for (size_t k = 0; k < v.arr.size(); ++k) {
                if (k) out += ",";
                out += dumpJson(v.arr[k]);
            }
            out += "]";
            return out;
        }
        case JsonValue::Object: {
            std::string out = "{";
            bool first = true;
            for (std::map<std::string, JsonValue>::const_iterator it = v.obj.begin();
                 it != v.obj.end(); ++it) {
                if (!first) out += ",";
                first = false;
                out += escapeString(it->first);
                out += ":";
                out += dumpJson(it->second);
            }
            out += "}";
            return out;
        }
    }
    return "null";
}

} // namespace gdei
