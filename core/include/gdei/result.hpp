// gdei/result.hpp
// 统一返回结果 / Common operation result.
#ifndef GDEI_RESULT_HPP
#define GDEI_RESULT_HPP

#include <string>

namespace gdei {

struct Outcome {
    bool ok = false;
    std::string message;   // 给人看的结果/错误信息，UTF-8 / human-readable, UTF-8
    int code = 0;          // 预留：细分错误码 / reserved error code

    static Outcome make(bool ok, const std::string& msg, int code = 0) {
        Outcome r; r.ok = ok; r.message = msg; r.code = code; return r;
    }
    static Outcome success(const std::string& msg) { return make(true, msg); }
    static Outcome failure(const std::string& msg, int code = 1) { return make(false, msg, code); }
};

} // namespace gdei

#endif // GDEI_RESULT_HPP
