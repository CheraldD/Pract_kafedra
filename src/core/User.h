#pragma once
#include <string>
#include "Role.h"

class User
{
public:
    User(const std::string &username, const std::string &rawPassword, Role role = Role::USER, unsigned int permissions = 0);
    
    User(
        const std::string &username,
        size_t passwordHash,
        Role role,
        bool isLocked,
        int failedLoginAttempts,
        unsigned int permissions);

    const std::string &getUsername() const;
    size_t getPasswordHash() const;
    Role getRole() const;
    bool isLocked() const;
    int getFailedLoginAttempts() const;
    unsigned int getPermissions() const; 

    void lock();
    void unlock();
    void incrementFailedAttempts();
    void resetFailedAttempts();

private:
    std::string m_username;
    size_t m_passwordHash; 
    Role m_role;
    bool m_isLocked;
    int m_failedLoginAttempts;
    unsigned int m_permissions; 
};