#include "User.h"
#include "../utils/Hash.h" 
#include "../services/PermissionManager.h" // Для доступа к enum Permission

User::User(const std::string& username, const std::string& rawPassword, Role role, unsigned int permissions)
    : m_username(username),
      m_passwordHash(hashPassword(rawPassword)),
      m_role(role),
      m_isLocked(false),
      m_failedLoginAttempts(0) 
{
    if (role == Role::ADMIN) {
        // Администратор всегда имеет все права, но для полноты установим маску
        m_permissions = static_cast<unsigned int>(Permission::READ) | 
                        static_cast<unsigned int>(Permission::WRITE) |
                        static_cast<unsigned int>(Permission::COPY_MOVE);
    } else {
        // Если права не указаны (permissions == 0), это саморегистрация. 
        // Даем права по умолчанию.
        if (permissions == 0) {
            m_permissions = static_cast<unsigned int>(Permission::READ) | static_cast<unsigned int>(Permission::WRITE);
        } else {
            // Иначе, это создание пользователя администратором с заданными правами.
            m_permissions = permissions;
        }
    }
}

User::User(const std::string& username, size_t passwordHash, Role role, bool isLocked, int failedLoginAttempts, unsigned int permissions)
    : m_username(username),
      m_passwordHash(passwordHash),
      m_role(role),
      m_isLocked(isLocked),
      m_failedLoginAttempts(failedLoginAttempts),
      m_permissions(permissions) {} // Инициализация нового поля

const std::string& User::getUsername() const {
    return m_username;
}

size_t User::getPasswordHash() const {
    return m_passwordHash;
}

Role User::getRole() const {
    return m_role;
}

bool User::isLocked() const {
    return m_isLocked;
}

int User::getFailedLoginAttempts() const {
    return m_failedLoginAttempts;
}

unsigned int User::getPermissions() const {
    return m_permissions;
}

void User::lock() {
    m_isLocked = true;
}

void User::unlock() {
    m_isLocked = false;
    resetFailedAttempts();
}

void User::incrementFailedAttempts() {
    m_failedLoginAttempts++;
}

void User::resetFailedAttempts() {
    m_failedLoginAttempts = 0;
}