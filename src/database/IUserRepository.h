#pragma once
#include "../trading/Types.h"
#include <optional>
#include <string>

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool save(const User& user) = 0;
    virtual std::optional<User> findById(const std::string& userId) = 0;
    virtual std::optional<User> findByUsername(const std::string& username) = 0;
};
