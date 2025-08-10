#pragma once
#include <string>
#include <sstream>
#include "../common/Errors.h"

inline std::string buildErrorJson(const Error& e) {
    std::ostringstream oss;
    oss << "{\"error\":{\"code\":\"" << toString(e.code)
        << "\",\"message\":\"";
    // naive escaping for quotes
    for(char c: e.message) { if (c=='"') oss << '\\'; oss << c; }
    oss << "\"";
    if(!e.details.empty()) {
        oss << ",\"details\":\"";
        for(char c: e.details) { if (c=='"') oss << '\\'; oss << c; }
        oss << "\"";
    }
    oss << "}}";
    return oss.str();
}
