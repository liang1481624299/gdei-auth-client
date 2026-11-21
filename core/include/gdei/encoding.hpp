// gdei/encoding.hpp
// 编码与字符集工具 / Encoding & charset helpers.
// 内部一律使用 UTF-8 字节串；在 Windows 上负责宽字符 <-> UTF-8 转换，
// so that non-ASCII (Chinese / accented) user names and passwords do not break.
#ifndef GDEI_ENCODING_HPP
#define GDEI_ENCODING_HPP

#include <string>
#include <vector>

namespace gdei {

// ---- Base64（标准字母表，与门户 JS 一致）/ standard Base64 ----
std::string base64Encode(const std::string& in);
// 解码；失败时 *ok 置 false 并返回尽量还原的内容。
std::string base64Decode(const std::string& in, bool* ok = 0);

// ---- URL 编码 / URL encode（表单用，保留字母数字 -_.~）----
std::string urlEncode(const std::string& in);
// URL 解码 / URL decode（支持 %XX 与 + -> 空格）。
std::string urlDecode(const std::string& in);

// ---- UTF-8 校验与归一 / UTF-8 check & normalize ----
bool isValidUtf8(const std::string& s);
// 若输入已是合法 UTF-8 原样返回；否则按 Latin-1 逐字节保留（不丢数据）。
// Keep bytes if it is not valid UTF-8, so we never silently corrupt input.
std::string ensureUtf8(const std::string& maybeLocal);

// ---- 本地配置口令混淆（可逆，仅避免明文落盘，不是强加密）----
// Obfuscation (reversible) just to keep plaintext out of the config file.
std::string obfuscate(const std::string& plainUtf8);
std::string deobfuscate(const std::string& ob);

#if defined(_WIN32)
// Windows 宽字符 <-> UTF-8 转换 / wide string <-> UTF-8 on Windows.
std::string  wideToUtf8(const std::wstring& w);
std::wstring utf8ToWide(const std::string& u8);
#endif

} // namespace gdei

#endif // GDEI_ENCODING_HPP
