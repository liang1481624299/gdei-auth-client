// src/core/BodyDecoder.hpp
// 旧版自助门户（H3C/NAC）响应体解码 / Legacy self-service portal body decoding.
//
// 新华三 8080 门户在部分部署仍以 GBK 输出 HTML，而客户端此前一律按 UTF-8 解码，
// 导致中文乱码。这里集中实现“声明缺失时的自适应解码”，供三个门户客户端复用。
// Some H3C 8080 deployments still emit GBK HTML; decoding them as UTF-8 produced
// mojibake. This helper centralises the fallback used by all portal clients.
#ifndef GDEI_BODY_DECODER_HPP
#define GDEI_BODY_DECODER_HPP

#include <QByteArray>
#include <QString>

namespace gdei {
namespace BodyDecoder {

// 未声明 charset 时的解码顺序 / Decoding order when charset is absent:
//   1. UTF-8 BOM
//   2. 严格 UTF-8（出现非法序列即判定失败）
//   3. GB18030（GBK/GB2312 的超集，覆盖简体中文门户）
//   4. UTF-8 宽松兜底（保证永不返回空串）
QString fromLegacy(const QByteArray& raw);

} // namespace BodyDecoder
} // namespace gdei

#endif // GDEI_BODY_DECODER_HPP
