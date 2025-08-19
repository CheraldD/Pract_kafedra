#include "UserManager.h"
#include "PermissionManager.h"
#include <memory>
#include <vector>

UserManager::UserManager(IUserRepository& repo, ILogger& logger)
    : m_userRepository(repo), m_logger(logger) {}

void UserManager::createUser(const std::string& username, const std::string& password) {
    if (username.length() < 3) {
        throw std::runtime_error("Имя пользователя должно быть не менее 3 символов.");
    }
    if (password.length() < 4) {
        throw std::runtime_error("Пароль должен быть не менее 4 символов.");
    }
    if (m_userRepository.findByUsername(username)) {
        throw std::runtime_error("Пользователь с таким именем уже существует.");
    }

    // Создаем пользователя с правами по умолчанию, заданными в конструкторе User
    auto newUser = std::make_shared<User>(username, password, Role::USER);
    m_userRepository.add(newUser);

    m_logger.log("Создан новый пользователь: '" + username + "'.");
}

// Новая реализация с параметром permissions
void UserManager::createUserByAdmin(const User& actor, const std::string& username, const std::string& password, unsigned int permissions) {
    PermissionManager::ensure(actor, Permission::CREATE_USER);

    if (username.length() < 3) {
        throw std::runtime_error("Имя пользователя должно быть не менее 3 символов.");
    }
    if (password.length() < 4) {
        throw std::runtime_error("Пароль должен быть не менее 4 символов.");
    }
    if (m_userRepository.findByUsername(username)) {
        throw std::runtime_error("Пользователь с таким именем уже существует.");
    }

    // Передаем выбранную администратором маску прав в конструктор
    auto newUser = std::make_shared<User>(username, password, Role::USER, permissions);
    m_userRepository.add(newUser);

    m_logger.log("Администратор '" + actor.getUsername() + "' создал нового пользователя: '" + username + "'.");
}


void UserManager::deleteUser(const User& actor, const std::string& usernameToDelete) {
    PermissionManager::ensure(actor, Permission::DELETE_USER);
    
    if (actor.getUsername() == usernameToDelete) {
        throw std::runtime_error("Вы не можете удалить свой собственный аккаунт.");
    }
    
    if (!m_userRepository.findByUsername(usernameToDelete)) {
        throw std::runtime_error("Пользователь с именем '" + usernameToDelete + "' не найден.");
    }

    m_userRepository.remove(usernameToDelete);
    
    m_logger.log("Пользователь '" + actor.getUsername() + "' удалил пользователя '" + usernameToDelete + "'.");
}

std::vector<std::shared_ptr<User>> UserManager::listAllUsers(const User& actor) {
    PermissionManager::ensure(actor, Permission::DELETE_USER);
    return m_userRepository.getAll();
}