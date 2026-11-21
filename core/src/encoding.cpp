// gdei/encoding.cpp
#include "gdei/encoding.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace gdei {

// ---------------------------------------------------------------- Base64 ----
static const char kB64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::string& in) {
    std::string out;
    unsigned int val = 0;
    int bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(kB64Table[(val >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) {
        out.push_back(kB64Table[((val << 8) >> (bits + 8)) & 0x3F]);
    }
    while (out.size() % 4) out.push_back('=');
    return out;
}

static int b64Val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::string base64Decode(const std::string& in, bool* ok) {
    if (ok) *ok = true;
    std::string out;
    int val = 0, bits = 0;
    for (char c : in) {
        if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
        int v = b64Val(c);
        if (v < 0) { if (ok) *ok = false; continue; }
        val = (val << 6) + v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
        }
    }
    return out;
}

// ------------------------------------------------------------------ URL ----
std::string urlEncode(const std::string& in) {
    std::string out;
    char buf[4];
    for (unsigned char c : in) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            std::snprintf(buf, sizeof(buf), "%%%02X", c);
            out.push_back(buf[0]); out.push_back(buf[1]); out.push_back(buf[2]);
        }
    }
    return out;
}

static int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string urlDecode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if (c == '+') { out.push_back(' '); }
        else if (c == '%' && i + 2 < in.size()) {
            int h = hexNibble(in[i + 1]);
            int l = hexNibble(in[i + 2]);
            if (h >= 0 && l >= 0) {
                out.push_back(static_cast<char>((h << 4) | l));
                i += 2;
            } else {
                out.push_back(c);
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// ---------------------------------------------------------------- UTF-8 ----
bool isValidUtf8(const std::string& s) {
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int len;
        if      (c <= 0x7F) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        else return false;
        if (i + len > s.size()) return false;
        for (int k = 1; k < len; ++k) {
            unsigned char n = static_cast<unsigned char>(s[i + k]);
            if ((n & 0xC0) != 0x80) return false;
        }
        i += len;
    }
    return true;
}

std::string ensureUtf8(const std::string& maybeLocal) {
    if (isValidUtf8(maybeLocal)) return maybeLocal;
    // 非 UTF-8 时按字节原样保留，避免二次转码乱码。
    return maybeLocal;
}

// ------------------------------------------------------------- obfuscate ----
static const char kObfKey[] = "gdei-portal-2026";

std::string obfuscate(const std::string& plainUtf8) {
    std::string xored;
    xored.reserve(plainUtf8.size());
    for (size_t i = 0; i < plainUtf8.size(); ++i) {
        xored.push_back(static_cast<char>(
            static_cast<unsigned char>(plainUtf8[i]) ^
            static_cast<unsigned char>(kObfKey[i % (sizeof(kObfKey) - 1)])));
    }
    return base64Encode(xored);
}

std::string deobfuscate(const std::string& ob) {
    if (ob.empty()) return std::string();
    bool ok = false;
    std::string xored = base64Decode(ob, &ok);
    if (!ok) return std::string();
    for (size_t i = 0; i < xored.size(); ++i) {
        xored[i] = static_cast<char>(
            static_cast<unsigned char>(xored[i]) ^
            static_cast<unsigned char>(kObfKey[i % (sizeof(kObfKey) - 1)]));
    }
    return xored;
}

#if defined(_WIN32)
std::string wideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                                static_cast<int>(w.size()), 0, 0, 0, 0);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                        &out[0], n, 0, 0);
    return out;
}

std::wstring utf8ToWide(const std::string& u8) {
    if (u8.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(),
                                static_cast<int>(u8.size()), 0, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), static_cast<int>(u8.size()),
                        &out[0], n);
    return out;
}
#endif

} // namespace gdei
