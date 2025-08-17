#pragma once
#include "../core/IUserRepository.h"
#include "../core/ILogger.h"
#include <memory>

class User; // Forward declaration

/**
 * @brief Отвечает за логику входа пользователя в систему.
 */
class Authenticator {
public:
    Authenticator(IUserRepository& repo, ILogger& logger, int maxAttempts = 3);

    /**
     * @brief Выполняет попытку входа пользователя в систему.
     * @return Умный указатель на объект User в случае успеха.
     * @throws AuthenticationException если аутентификация не удалась (например,
     *         UserNotFoundException, AccountLockedException, InvalidCredentialsException).
     */
    std::shared_ptr<User> login(const std::string& username, const std::string& password) noexcept(false);

private:
    IUserRepository& m_userRepository;
    ILogger& m_logger;
    const int m_maxFailedAttempts;
};