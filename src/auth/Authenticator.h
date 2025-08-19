#pragma once
#include "../core/IUserRepository.h"
#include "../core/ILogger.h"
#include <memory>

// Forward declaration для уменьшения зависимостей в заголовочных файлах
class User; 
struct SystemSettings;

/**
 * @brief Отвечает за логику входа пользователя в систему.
 */
class Authenticator {
public:
    // Изменен конструктор для приема объекта настроек по ссылке
    Authenticator(IUserRepository& repo, ILogger& logger, SystemSettings& settings);

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
    // Храним ссылку на настройки, чтобы всегда иметь актуальное значение
    SystemSettings& m_settings;
};