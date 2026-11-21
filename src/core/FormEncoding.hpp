// src/core/FormEncoding.hpp
// 将 UTF-8 percent-encoded 表单按请求配置重新序列化。
// Reserialize UTF-8 percent-encoded forms using the configured request charset.
#ifndef GDEI_FORM_ENCODING_HPP
#define GDEI_FORM_ENCODING_HPP

#include <QByteArray>
#include <QString>

namespace gdei {
namespace FormEncoding {

enum class Error {
    None,
    NonAscii,
    Unmappable,
    InvalidForm
};

struct Result {
    QByteArray body;
    QString charset;
    Error error = Error::None;
    QString errorKey;
};

Result encode(const QString& body, const QString& encoding);

} // namespace FormEncoding
} // namespace gdei

#endif // GDEI_FORM_ENCODING_HPP
