#include "User.h"
#include "../utils/Hash.h" // Подключаем нашу утилиту для хеширования

User::User(const std::string& username, const std::string& rawPassword, Role role)
    : m_username(username),
      m_passwordHash(hashPassword(rawPassword)), // Хешируем пароль при создании
      m_role(role),
      m_isLocked(false),
      m_failedLoginAttempts(0) {}
User::User(const std::string& username, size_t passwordHash, Role role, bool isLocked, int failedLoginAttempts)
    : m_username(username),
      m_passwordHash(passwordHash),
      m_role(role),
      m_isLocked(isLocked),
      m_failedLoginAttempts(failedLoginAttempts) {}
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

void User::lock() {
    m_isLocked = true;
}

void User::unlock() {
    m_isLocked = false;
    // При разблокировке также сбрасываем счетчик неудачных попыток
    resetFailedAttempts();
}

void User::incrementFailedAttempts() {
    m_failedLoginAttempts++;
}

void User::resetFailedAttempts() {
    m_failedLoginAttempts = 0;
}