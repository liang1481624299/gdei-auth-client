// src/core/SelfServiceCodec.hpp
// 自助门户口令前端编码 / Self-service portal client-side password encoding.
//
// 复刻登录页 ssv.js 的 imc_byod_function_base64_with_chinese：
//  1) 逐字符 escape：中文（码位 >255）转义为字面 "\uXXXX"（大写十六进制），
//     其余字符原样保留，反斜杠双写；
//  2) 对转义后的字符串按字符低 8 位做标准 Base64（标准字母表 + '=' 填充）。
// Mirrors ssv.js: escape per char (non-BMP -> literal "\uXXXX", backslash doubled),
// then standard Base64 over the low byte of each char.
//
// 注意：这是前端可逆编码，不是服务端强加密；仅用于对齐门户提交格式。
// This is a reversible client-side encoding, not strong server-side crypto.
#ifndef GDEI_SELF_SERVICE_CODEC_HPP
#define GDEI_SELF_SERVICE_CODEC_HPP

#include <QString>

namespace gdei {
namespace SelfServiceCodec {

// 与门户前端一致的口令编码 / Password encoding matching the portal frontend.
QString encodePassword(const QString& input);

} // namespace SelfServiceCodec
} // namespace gdei

#endif // GDEI_SELF_SERVICE_CODEC_HPP
