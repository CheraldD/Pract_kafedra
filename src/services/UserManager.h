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

    void deleteUser(const User& actor, const std::string& usernameToDelete);

    /**
     * @brief Возвращает список всех пользователей.
     * @param actor Пользователь, выполняющий действие (для проверки прав).
     * @return Вектор с указателями на пользователей.
     * @throws std::runtime_error если у пользователя нет прав на просмотр списка.
     */
    std::vector<std::shared_ptr<User>> listAllUsers(const User& actor);

private:
    IUserRepository& m_userRepository;
    ILogger& m_logger;
};