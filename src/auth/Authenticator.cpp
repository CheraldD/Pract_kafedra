#include "Authenticator.h"
#include "../core/User.h"
#include "../core/SystemSettings.h" // Подключаем определение SystemSettings
#include "../utils/Hash.h"
#include "AuthExceptions.h"

// Конструктор теперь принимает SystemSettings по ссылке
Authenticator::Authenticator(IUserRepository& repo, ILogger& logger, SystemSettings& settings)
    : m_userRepository(repo),
      m_logger(logger),
      m_settings(settings) {} // Сохраняем ссылку

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

    if (user->getPasswordHash() == hashPassword(password)) {
        m_logger.log("Пользователь '" + username + "' успешно вошел в систему.");
        if (user->getFailedLoginAttempts() > 0) {
            user->resetFailedAttempts();
            m_userRepository.update(user);
        }
        return user;
    } 
    else {
        if (user->getRole() == Role::ADMIN) {
            m_logger.log("!!! ВНИМАНИЕ: Неудачная попытка входа под учетной записью АДМИНИСТРАТОРА '" + username + "'.");
        } else {
            user->incrementFailedAttempts();
            // Используем настраиваемое значение из m_settings
            const int maxAttempts = m_settings.maxLoginAttempts;
            m_logger.log("Неудачная попытка входа для пользователя '" + username + 
                         "'. Попытка " + std::to_string(user->getFailedLoginAttempts()) + 
                         " из " + std::to_string(maxAttempts) + ".");

            if (user->getFailedLoginAttempts() >= maxAttempts) {
                user->lock();
                m_logger.log("Аккаунт пользователя '" + username + "' заблокирован из-за большого количества неудачных попыток входа.");
            }
            m_userRepository.update(user);
        }
        
        throw InvalidCredentialsException("Неверный пароль.");
    }
}