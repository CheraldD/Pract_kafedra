#pragma once
#include "../core/IUserRepository.h"
#include "../core/ILogger.h"
#include <memory>
class User; 
struct SystemSettings;
class Authenticator {
public:
    Authenticator(IUserRepository& repo, ILogger& logger, SystemSettings& settings);
    std::shared_ptr<User> login(const std::string& username, const std::string& password) noexcept(false);

private:
    IUserRepository& m_userRepository;
    ILogger& m_logger;
    SystemSettings& m_settings;
};