#include "Authenticator.h"
#include "../core/User.h"
#include "../utils/Hash.h"
#include "AuthExceptions.h"

Authenticator::Authenticator(IUserRepository& repo, ILogger& logger, int maxAttempts)
    : m_userRepository(repo),
      m_logger(logger),
      m_maxFailedAttempts(maxAttempts) {}

std::shared_ptr<User> Authenticator::login(const std::string& username, const std::string& password) noexcept(false) {
    auto user = m_userRepository.findByUsername(username);

    if (!user) {
        m_logger.log("Неудачная попытка входа: пользователь '" + username + "' не найден.");
        throw UserNotFoundException("Пользователь с таким именем не найден.");
    }

    if (user->isLocked()) {
        m_logger.log("Попытка входа в заблокированный аккаунт: '" + username + "'.");
        throw AccountLockedException("Этот аккаунт заблокирован. Обратитесь к администратору.");
    }

    // Если пароль верный
    if (user->getPasswordHash() == hashPassword(password)) {
        m_logger.log("Пользователь '" + username + "' успешно вошел в систему.");
        if (user->getFailedLoginAttempts() > 0) {
            user->resetFailedAttempts();
            m_userRepository.update(user);
        }
        return user;
    } 
    // Если пароль неверный
    else {
        // ИЗМЕНЕНИЕ: Добавляем проверку роли перед блокировкой
        if (user->getRole() == Role::ADMIN) {
            // Если это админ, просто логируем ошибку, но не блокируем
            m_logger.log("!!! ВНИМАНИЕ: Неудачная попытка входа под учетной записью АДМИНИСТРАТОРА '" + username + "'.");
        } else {
            // Для обычных пользователей оставляем старую логику
            user->incrementFailedAttempts();
            m_logger.log("Неудачная попытка входа для пользователя '" + username + 
                         "'. Попытка " + std::to_string(user->getFailedLoginAttempts()) + 
                         " из " + std::to_string(m_maxFailedAttempts) + ".");

            if (user->getFailedLoginAttempts() >= m_maxFailedAttempts) {
                user->lock();
                m_logger.log("Аккаунт пользователя '" + username + "' заблокирован из-за большого количества неудачных попыток входа.");
            }
            m_userRepository.update(user);
        }
        
        throw InvalidCredentialsException("Неверный пароль.");
    }
}