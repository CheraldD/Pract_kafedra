#pragma once
#include "../core/IUserRepository.h"
#include "../core/ILogger.h"
#include "../core/User.h"
#include <string>
#include <vector>
#include <memory>

class UserManager {
public:
    UserManager(IUserRepository& repo, ILogger& logger);

    void createUser(const std::string& username, const std::string& password);

    // Добавлен параметр permissions для указания прав при создании
    void createUserByAdmin(const User& actor, const std::string& username, const std::string& password, unsigned int permissions);

    void deleteUser(const User& actor, const std::string& usernameToDelete);

    std::vector<std::shared_ptr<User>> listAllUsers(const User& actor);

private:
    IUserRepository& m_userRepository;
    ILogger& m_logger;
};