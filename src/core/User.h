#pragma once
#include <string>
#include "Role.h"

class User
{
public:
    // Конструктор принимает пароль в открытом виде и сразу хеширует его
    User(const std::string &username, const std::string &rawPassword, Role role = Role::USER);
    User(
        const std::string &username,
        size_t passwordHash,
        Role role,
        bool isLocked,
        int failedLoginAttempts);
    // Getters - методы для получения доступа к полям класса
    const std::string &getUsername() const;
    size_t getPasswordHash() const;
    Role getRole() const;
    bool isLocked() const;
    int getFailedLoginAttempts() const;

    // Методы для изменения состояния объекта
    void lock();
    void unlock();
    void incrementFailedAttempts();
    void resetFailedAttempts();

private:
    std::string m_username;
    size_t m_passwordHash; // Храним только хеш пароля
    Role m_role;
    bool m_isLocked;
    int m_failedLoginAttempts;
};