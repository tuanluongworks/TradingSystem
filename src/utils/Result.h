#pragma once
#include <variant>
#include <utility>
#include <type_traits>
#include "../common/Errors.h"

// Lightweight Result type (until std::expected available everywhere)
template <typename T>
class Result {
public:
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    Result(const Error& err) : data_(err) {}
    Result(Error&& err) : data_(std::move(err)) {}

    bool hasValue() const { return std::holds_alternative<T>(data_); }
    explicit operator bool() const { return hasValue(); }

    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }

    const Error& error() const { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

// Void specialization using bool success marker
class ResultVoid {
public:
    ResultVoid() : ok_(true) {}
    ResultVoid(Error err) : ok_(false), err_(std::move(err)) {}
    bool hasValue() const { return ok_; }
    explicit operator bool() const { return ok_; }
    const Error& error() const { return *err_; }
private:
    bool ok_{false};
    std::optional<Error> err_{};
};
