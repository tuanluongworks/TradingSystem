#pragma once
#include <string>
#include <utility>
#include <optional>
#include <string_view>

enum class ErrorCode {
    Validation,
    Auth,
    NotFound,
    Conflict,
    RateLimit,
    Internal,
    Unauthorized
};

struct Error {
    ErrorCode code{ErrorCode::Internal};
    std::string message;
    std::string details;
    static Error validation(std::string msg, std::string details = {}) { return {ErrorCode::Validation, std::move(msg), std::move(details)}; }
    static Error auth(std::string msg, std::string details = {}) { return {ErrorCode::Auth, std::move(msg), std::move(details)}; }
    static Error notFound(std::string msg, std::string details = {}) { return {ErrorCode::NotFound, std::move(msg), std::move(details)}; }
    static Error conflict(std::string msg, std::string details = {}) { return {ErrorCode::Conflict, std::move(msg), std::move(details)}; }
    static Error rateLimit(std::string msg, std::string details = {}) { return {ErrorCode::RateLimit, std::move(msg), std::move(details)}; }
    static Error internal(std::string msg, std::string details = {}) { return {ErrorCode::Internal, std::move(msg), std::move(details)}; }
    static Error unauthorized(std::string msg, std::string details = {}) { return {ErrorCode::Unauthorized, std::move(msg), std::move(details)}; }
};

inline std::string_view toString(ErrorCode c) {
    switch(c) {
        case ErrorCode::Validation: return "validation";
        case ErrorCode::Auth: return "auth";
        case ErrorCode::NotFound: return "not_found";
        case ErrorCode::Conflict: return "conflict";
        case ErrorCode::RateLimit: return "rate_limit";
        case ErrorCode::Internal: return "internal";
        case ErrorCode::Unauthorized: return "unauthorized";
    }
    return "internal";
}
