# Полный Исходный Код Проекта Системы Управления Доступом
_Сгенерировано: 2025-08-19 20:42:16_

## Содержание
- [`src/auth/AuthExceptions.h`](#srcauthAuthExceptionsh)
- [`src/auth/Authenticator.cpp`](#srcauthAuthenticatorcpp)
- [`src/auth/Authenticator.h`](#srcauthAuthenticatorh)
- [`src/cli/CLI.cpp`](#srccliCLIcpp)
- [`src/cli/CLI.h`](#srccliCLIh)
- [`src/core/ILogger.h`](#srccoreILoggerh)
- [`src/core/IUserRepository.h`](#srccoreIUserRepositoryh)
- [`src/core/Role.h`](#srccoreRoleh)
- [`src/core/SystemSettings.h`](#srccoreSystemSettingsh)
- [`src/core/User.cpp`](#srccoreUsercpp)
- [`src/core/User.h`](#srccoreUserh)
- [`src/infrastructure/FileLogger.cpp`](#srcinfrastructureFileLoggercpp)
- [`src/infrastructure/FileLogger.h`](#srcinfrastructureFileLoggerh)
- [`src/infrastructure/FileUserRepository.cpp`](#srcinfrastructureFileUserRepositorycpp)
- [`src/infrastructure/FileUserRepository.h`](#srcinfrastructureFileUserRepositoryh)
- [`src/main.cpp`](#srcmaincpp)
- [`src/services/FileManager.cpp`](#srcservicesFileManagercpp)
- [`src/services/FileManager.h`](#srcservicesFileManagerh)
- [`src/services/PermissionManager.h`](#srcservicesPermissionManagerh)
- [`src/services/UserManager.cpp`](#srcservicesUserManagercpp)
- [`src/services/UserManager.h`](#srcservicesUserManagerh)
- [`src/utils/Hash.cpp`](#srcutilsHashcpp)
- [`src/utils/Hash.h`](#srcutilsHashh)

---

## <a name="srcauthAuthExceptionsh"></a>Файл: `src/auth/AuthExceptions.h`

```cpp
#pragma once
#include <stdexcept>

/**
 * @brief Базовый класс для всех исключений аутентификации.
 */
class AuthenticationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Исключение, выбрасываемое, когда пользователь не найден.
 */
class UserNotFoundException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};

/**
 * @brief Исключение, выбрасываемое, когда аккаунт пользователя заблокирован.
 */
class AccountLockedException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};

/**
 * @brief Исключение, выбрасываемое, когда предоставлен неверный пароль.
 */
class InvalidCredentialsException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};
```

---

## <a name="srcauthAuthenticatorcpp"></a>Файл: `src/auth/Authenticator.cpp`

```cpp
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
```

---

## <a name="srcauthAuthenticatorh"></a>Файл: `src/auth/Authenticator.h`

```cpp
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
```

---

## <a name="srccliCLIcpp"></a>Файл: `src/cli/CLI.cpp`

```cpp
#include "CLI.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <termios.h>

#include "../auth/Authenticator.h"
#include "../auth/AuthExceptions.h"
#include "../services/UserManager.h"
#include "../services/FileManager.h"
#include "../services/PermissionManager.h" // Подключаем для enum-ов
#include "../core/User.h"
#include "../core/SystemSettings.h"

namespace {
    // --- Цветовые ANSI-коды для терминала ---
    namespace Color {
        const std::string RESET = "\033[0m";
        const std::string BOLD = "\033[1m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string BRIGHT_RED = "\033[91m";
        const std::string BRIGHT_GREEN = "\033[92m";
        const std::string BRIGHT_YELLOW = "\033[93m";
    }

    const std::string CANCEL_COMMAND = "cancel";
    const std::string MENU_SEPARATOR = "---";

    void clearInputBuffer() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string getMaskedPassword() {
        std::string password;
        char ch;
        
        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n') {
            if (ch == 127 || ch == 8) { // Backspace
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else {
                password += ch;
                std::cout << '*' << std::flush;
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << std::endl;
        return password;
    }

    size_t count_utf8_chars(const std::string& str) {
        size_t length = 0;
        for (unsigned char c : str) {
            if ((c & 0xC0) != 0x80) {
                length++;
            }
        }
        return length;
    }

    void displayMenu(const std::string& title, const std::vector<std::string>& items) {
        size_t maxWidth = count_utf8_chars(title);
        for (const auto& item : items) {
            if (item != MENU_SEPARATOR) {
                maxWidth = std::max(maxWidth, count_utf8_chars(item));
            }
        }

        std::string h_line;
        for(size_t i = 0; i < maxWidth + 2; ++i) h_line += "═";

        std::cout << "\n" << Color::BRIGHT_YELLOW << "╔" << h_line << "╗\n";
        
        size_t titlePad = maxWidth - count_utf8_chars(title);
        std::cout << "║ " << Color::BOLD << Color::CYAN << title << std::string(titlePad, ' ') << Color::RESET << Color::BRIGHT_YELLOW << " ║\n";
        
        std::cout << "╠" << h_line << "╣\n";
        
        for (const auto& item : items) {
            if (item == MENU_SEPARATOR) {
                std::cout << "╠" << h_line << "╣\n";
            } else {
                size_t itemPad = maxWidth - count_utf8_chars(item);
                size_t numEndPos = item.find(". ");
                if (numEndPos != std::string::npos) {
                     std::cout << "║ " << Color::BRIGHT_YELLOW << item.substr(0, numEndPos + 1) << Color::RESET
                              << item.substr(numEndPos + 1) << std::string(itemPad, ' ') << Color::BRIGHT_YELLOW << " ║\n";
                } else {
                     std::cout << "║ " << Color::RESET << item << std::string(itemPad, ' ') << Color::BRIGHT_YELLOW << " ║\n";
                }
            }
        }
        
        std::cout << "╚" << h_line << "╝" << Color::RESET << std::endl;
    }

} // namespace

CLI::CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager, SystemSettings& settings)
    : m_auth(auth),
      m_userManager(userManager),
      m_fileManager(fileManager),
      m_settings(settings),
      m_currentUser(nullptr),
      m_shouldRun(true) {}

void CLI::run() {
    while (m_shouldRun) {
        if (!m_currentUser) {
            handleAuthScreen();
        }
        if (m_currentUser) {
            handleUserActions();
        }
    }
    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Завершение работы. До свидания!" << std::endl;
}

void CLI::handleAuthScreen() {
    displayMenu("Система Управления Доступом", {
        "1. Вход",
        "2. Регистрация",
        MENU_SEPARATOR,
        "0. Выход"
    });
    
    std::cout << Color::BRIGHT_YELLOW << "> " << Color::RESET << std::flush;
    int choice;
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Некорректный ввод. Пожалуйста, введите число." << std::endl;
        std::cin.clear();
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    switch (choice) {
        case 1: handleLogin(); break;
        case 2: handleRegistration(); break;
        case 0: m_shouldRun = false; break;
        default: std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Неизвестная команда." << std::endl; break;
    }
}

void CLI::handleLogin() {
    std::cout << "\n" << Color::BLUE << "--- Вход в систему ---" << Color::RESET << std::endl;
    std::string username, password;

    std::cout << "Имя пользователя: " << std::flush;
    std::getline(std::cin, username);
    
    std::cout << "Пароль: " << std::flush;
    password = getMaskedPassword(); 
    
    try {
        m_currentUser = m_auth.login(username, password);
        std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Добро пожаловать, " 
                  << Color::BOLD << m_currentUser->getUsername() << Color::RESET << "!" << std::endl;
    } 
    catch (const AuthenticationException& e) {
        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Ошибка входа: " << e.what() << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Произошла непредвиденная системная ошибка: " << e.what() << std::endl;
    }
}

void CLI::handleRegistration() {
    std::cout << "\n" << Color::BLUE << "--- Регистрация нового пользователя ---" << Color::RESET << std::endl;
    std::string username, password, passwordConfirm;
    
    std::cout << "Введите новое имя пользователя: " << std::flush;
    std::getline(std::cin, username);
    
    std::cout << "Введите пароль (мин. 4 символа): " << std::flush;
    password = getMaskedPassword();

    std::cout << "Подтвердите пароль: " << std::flush;
    passwordConfirm = getMaskedPassword();

    if (password != passwordConfirm) {
        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Ошибка: пароли не совпадают." << std::endl;
        return;
    }

    try {
        m_userManager.createUser(username, password);
        std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Пользователь '" 
                  << Color::BOLD << username << Color::RESET << "' успешно зарегистрирован. Теперь вы можете войти." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Ошибка регистрации: " << e.what() << std::endl;
    }
}

void CLI::showMainMenu() const {
    const std::string title = "Меню (Пользователь: " + m_currentUser->getUsername() + ")";
    std::vector<std::string> items = {
        "1. Прочитать файл",
        "2. Записать в файл",
        "3. Копировать файл",
        "4. Переместить файл"
    };
    if (m_currentUser->getRole() == Role::ADMIN) {
        items.push_back(MENU_SEPARATOR);
        items.push_back("5. Удалить пользователя (Админ)");
        items.push_back("6. Создать пользователя (Админ)");
        items.push_back("7. Настройки системы (Админ)");
    }
    items.push_back(MENU_SEPARATOR);
    items.push_back("9. Выйти из аккаунта");
    items.push_back("0. Выйти из приложения");

    displayMenu(title, items);
}

void CLI::handleSystemSettings() {
    std::cout << "\n" << Color::BLUE << "--- Настройки Системы ---" << Color::RESET << std::endl;
    std::cout << "Текущее макс. кол-во попыток входа: " << Color::BOLD << m_settings.maxLoginAttempts << Color::RESET << std::endl;
    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите новое значение (например, 5) или '" << CANCEL_COMMAND << "' для отмены: " << std::flush;

    std::string input;
    std::getline(std::cin, input);
    if (input == CANCEL_COMMAND || input.empty()) {
        std::cout << "Отмена." << std::endl;
        return;
    }
    
    try {
        int newMaxAttempts = std::stoi(input);
        if (newMaxAttempts <= 0) {
            std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Значение должно быть положительным числом." << std::endl;
            return;
        }
        m_settings.maxLoginAttempts = newMaxAttempts;
        m_settings.save(); // Сохраняем новое значение в файл
        
        std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Максимальное количество попыток входа изменено на " 
                  << Color::BOLD << m_settings.maxLoginAttempts << Color::RESET << "." << std::endl;

    } catch (const std::invalid_argument&) {
        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Некорректный ввод. Пожалуйста, введите целое число." << std::endl;
    } catch (const std::out_of_range&) {
        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Введенное число слишком велико." << std::endl;
    }
}


void CLI::handleUserActions() {
    int choice = -1;
    while (m_currentUser && m_shouldRun) {
        showMainMenu();
        std::cout << Color::BRIGHT_YELLOW << "> " << Color::RESET << std::flush;
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Некорректный ввод. Пожалуйста, введите число." << std::endl;
            std::cin.clear();
            clearInputBuffer();
            choice = -1;
            continue;
        }
        
        clearInputBuffer();

        try {
            switch (choice) {
                case 1: { // Прочитать файл
                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к файлу для чтения (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;
                    m_fileManager.readFile(*m_currentUser, path);
                    break;
                }
                case 2: { // Записать в файл
                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к файлу для записи (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите содержимое для дозаписи (одной строкой): " << std::flush;
                    std::string content;
                    std::getline(std::cin, content);
                    m_fileManager.writeFile(*m_currentUser, path, content);
                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Данные успешно добавлены в файл '" << path << "'." << std::endl;
                    break;
                }
                case 3: case 4: { // Копировать или переместить
                     std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к исходному файлу (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string source;
                    std::getline(std::cin, source);
                    if (source == CANCEL_COMMAND || source.empty()) break;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к файлу/папке назначения: " << std::flush;
                    std::string dest;
                    std::getline(std::cin, dest);
                    if (dest == CANCEL_COMMAND || dest.empty()) break;
                    
                    if (choice == 3) {
                         m_fileManager.copyFile(*m_currentUser, source, dest);
                         std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Файл '" << source << "' успешно скопирован." << std::endl;
                    } else {
                         m_fileManager.moveFile(*m_currentUser, source, dest);
                         std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Файл '" << source << "' успешно перемещен." << std::endl;
                    }
                    break;
                }
                case 5: { // Удалить пользователя (Админ)
                    if (!PermissionManager::has(*m_currentUser, Permission::DELETE_USER)) {
                         std::cout << "\n" << Color::RED << "[✗] " << Color::RESET << "Неизвестная команда." << std::endl; break;
                    }
                    std::cout << "\n" << Color::BLUE << "--- Список Пользователей ---" << Color::RESET << std::endl;
                    const auto users = m_userManager.listAllUsers(*m_currentUser);
                    for (const auto& user : users) {
                        std::cout << "- " << Color::BOLD << user->getUsername() << Color::RESET
                                  << " (Роль: " << (user->getRole() == Role::ADMIN ? Color::MAGENTA : Color::GREEN) 
                                  << (user->getRole() == Role::ADMIN ? "Админ" : "Пользователь") << Color::RESET << ")"
                                  << (user->isLocked() ? Color::BRIGHT_RED + " [ЗАБЛОКИРОВАН]" + Color::RESET : "") << std::endl;
                    }
                    std::cout << Color::BLUE << "--------------------------\n" << Color::RESET;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите имя пользователя для удаления (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string usernameToDelete;
                    std::getline(std::cin, usernameToDelete);
                    if (usernameToDelete == CANCEL_COMMAND || usernameToDelete.empty()) break;

                    m_userManager.deleteUser(*m_currentUser, usernameToDelete);
                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Пользователь '" << usernameToDelete << "' успешно удален." << std::endl;
                    break;
                }
                case 6: { // Создать пользователя (Админ)
                    if (!PermissionManager::has(*m_currentUser, Permission::CREATE_USER)) {
                        std::cout << "\n" << Color::RED << "[✗] " << Color::RESET << "Неизвестная команда." << std::endl; break;
                    }
                    std::cout << "\n" << Color::BLUE << "--- Создание нового пользователя ---" << Color::RESET << std::endl;
                    std::string newUsername, newPassword;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите имя нового пользователя (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::getline(std::cin, newUsername);
                    if (newUsername == CANCEL_COMMAND || newUsername.empty()) break;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите пароль (мин. 4 символа): " << std::flush;
                    newPassword = getMaskedPassword();

                    std::cout << Color::BLUE << "\n--- Выбор прав для пользователя ---" << Color::RESET << std::endl;
                    std::cout << "1. Чтение и запись" << std::endl;
                    std::cout << "2. Копирование и перемещение" << std::endl;
                    std::cout << "3. Все права (чтение, запись, копирование, перемещение)" << std::endl;
                    std::cout << Color::BRIGHT_YELLOW << "> " << Color::RESET << std::flush;
                    
                    int permChoice;
                    std::cin >> permChoice;
                    
                    if (std::cin.fail()) {
                        std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Некорректный ввод." << std::endl;
                        std::cin.clear();
                        clearInputBuffer();
                        break;
                    }
                    clearInputBuffer();
                    
                    unsigned int permissions = static_cast<unsigned int>(Permission::NONE);
                    switch (permChoice) {
                        case 1:
                            permissions = static_cast<unsigned int>(Permission::READ) | static_cast<unsigned int>(Permission::WRITE);
                            break;
                        case 2:
                            permissions = static_cast<unsigned int>(Permission::COPY_MOVE);
                            break;
                        case 3:
                            permissions = static_cast<unsigned int>(Permission::READ) | static_cast<unsigned int>(Permission::WRITE) | static_cast<unsigned int>(Permission::COPY_MOVE);
                            break;
                        default:
                            std::cout << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Неверный выбор. Операция отменена." << std::endl;
                            break;
                    }

                    if (permissions == static_cast<unsigned int>(Permission::NONE)) {
                        break; 
                    }
                    
                    m_userManager.createUserByAdmin(*m_currentUser, newUsername, newPassword, permissions);

                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Пользователь '" << newUsername << "' успешно создан." << std::endl;
                    break;
                }
                case 7: { // Настройки системы (Админ)
                     if (m_currentUser->getRole() != Role::ADMIN) {
                        std::cout << "\n" << Color::RED << "[✗] " << Color::RESET << "Неизвестная команда." << std::endl; break;
                    }
                    handleSystemSettings();
                    break;
                }
                case 9: // Выход из аккаунта
                    m_currentUser = nullptr;
                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Вы вышли из системы." << std::endl;
                    break;
                case 0: // Выход из приложения
                    m_shouldRun = false;
                    m_currentUser = nullptr;
                    break;
                default:
                    std::cout << "\n" << Color::RED << "[✗] " << Color::RESET << "Неизвестная команда. Попробуйте еще раз." << std::endl;
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "\n" << Color::BRIGHT_RED << "[✗] " << Color::RESET << "Операция не удалась: " << e.what() << std::endl;
        }
    }
}
```

---

## <a name="srccliCLIh"></a>Файл: `src/cli/CLI.h`

```cpp
#pragma once
#include <memory>

// Forward declarations
class Authenticator;
class UserManager;
class FileManager;
class User;
struct SystemSettings;

class CLI {
public:
    // Конструктор принимает новый параметр - SystemSettings
    CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager, SystemSettings& settings);
    void run();

private:
    void handleAuthScreen();
    void handleLogin();
    void handleRegistration();
    void handleUserActions();
    void showMainMenu() const;

    // Новый метод для управления настройками
    void handleSystemSettings();

    Authenticator& m_auth;
    UserManager& m_userManager;
    FileManager& m_fileManager;
    // Ссылка на настройки, чтобы CLI мог их изменять
    SystemSettings& m_settings; 
    std::shared_ptr<User> m_currentUser;
    bool m_shouldRun;
};
```

---

## <a name="srccoreILoggerh"></a>Файл: `src/core/ILogger.h`

```cpp
#pragma once
#include <string>

/**
 * @brief Абстрактный интерфейс для системы журналирования (логгера).
 * Определяет контракт, которому должны следовать все конкретные реализации логгеров.
 */
class ILogger {
public:
    // Виртуальный деструктор обязателен для базовых классов с виртуальными функциями
    virtual ~ILogger() = default;

    /**
     * @brief Записывает сообщение в журнал.
     * @param message Сообщение для записи.
     */
    virtual void log(const std::string& message) = 0;
};
```

---

## <a name="srccoreIUserRepositoryh"></a>Файл: `src/core/IUserRepository.h`

```cpp
#pragma once
#include <string>
#include <memory>
#include <vector>

class User; 

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    
    virtual std::shared_ptr<User> findByUsername(const std::string& username) = 0;

    // ОБЯЗАТЕЛЬНОЕ ДОБАВЛЕНИЕ: объявляем метод в интерфейсе
    virtual std::vector<std::shared_ptr<User>> getAll() = 0;

    virtual void add(std::shared_ptr<User> user) = 0;
    
    virtual void update(std::shared_ptr<User> user) = 0;

    virtual void remove(const std::string& username) = 0;
};
```

---

## <a name="srccoreRoleh"></a>Файл: `src/core/Role.h`

```cpp
#pragma once

// Используем enum class для строгой типизации ролей и предотвращения неявных
// преобразований в int. Это повышает безопасность типов.
enum class Role {
    USER,  // Обычный пользователь
    ADMIN  // Администратор
};
```

---

## <a name="srccoreSystemSettingsh"></a>Файл: `src/core/SystemSettings.h`

```cpp
#pragma once
#include <string>
#include <fstream>
#include <iostream>

/**
 * @brief Структура для хранения глобальных настроек системы.
 * Это позволяет централизованно управлять параметрами, которые могут
 * изменяться администратором во время выполнения программы.
 */
struct SystemSettings {
    /**
     * @brief Максимальное количество последовательных неудачных попыток входа,
     * после которого аккаунт пользователя будет заблокирован.
     * Значение по умолчанию: 3.
     */
    int maxLoginAttempts = 3;

private:
    std::string m_configFilePath; // Путь к файлу для сохранения настроек

public:
    /**
     * @brief Загружает настройки из файла. Если файл не существует или пуст,
     * используются значения по умолчанию.
     * @param path Путь к файлу настроек.
     */
    void load(const std::string& path) {
        m_configFilePath = path;
        std::ifstream file(m_configFilePath);
        if (file.is_open() && (file >> maxLoginAttempts)) {
            // Значение успешно прочитано
            if (maxLoginAttempts <= 0) {
                maxLoginAttempts = 3; // Восстанавливаем безопасное значение по умолчанию
            }
        }
        // Если файл не открылся или пуст, просто используем значение по умолчанию.
    }

    /**
     * @brief Сохраняет текущие настройки в файл.
     */
    void save() const {
        if (m_configFilePath.empty()) {
            return; // Некуда сохранять, если путь не был задан
        }
        // Открываем файл для перезаписи (trunc)
        std::ofstream file(m_configFilePath, std::ios_base::trunc);
        if (file.is_open()) {
            file << maxLoginAttempts;
        } else {
            // В случае ошибки выводим предупреждение. Логгер здесь недоступен.
            std::cerr << "Предупреждение: Не удалось сохранить файл настроек: " << m_configFilePath << std::endl;
        }
    }
};
```

---

## <a name="srccoreUsercpp"></a>Файл: `src/core/User.cpp`

```cpp
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
```

---

## <a name="srccoreUserh"></a>Файл: `src/core/User.h`

```cpp
#pragma once
#include <string>
#include "Role.h"

class User
{
public:
    // Конструктор для регистрации (пользователем) и создания (администратором)
    // permissions = 0 означает, что будут установлены права по умолчанию для саморегистрации
    User(const std::string &username, const std::string &rawPassword, Role role = Role::USER, unsigned int permissions = 0);
    
    // Конструктор для загрузки пользователя из хранилища
    User(
        const std::string &username,
        size_t passwordHash,
        Role role,
        bool isLocked,
        int failedLoginAttempts,
        unsigned int permissions);

    // Getters
    const std::string &getUsername() const;
    size_t getPasswordHash() const;
    Role getRole() const;
    bool isLocked() const;
    int getFailedLoginAttempts() const;
    unsigned int getPermissions() const; // Новый getter для прав

    // Методы для изменения состояния объекта
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
    unsigned int m_permissions; // Новое поле для битовой маски прав
};
```

---

## <a name="srcinfrastructureFileLoggercpp"></a>Файл: `src/infrastructure/FileLogger.cpp`

```cpp
#include "FileLogger.h"
#include <chrono>    // для получения текущего времени
#include <iomanip>   // для форматирования времени (std::put_time)
#include <stdexcept> // для генерации исключений (std::runtime_error)
#include <ctime>     // для time_t и struct tm

FileLogger::FileLogger(const std::string& filePath) {
    // Открываем файл в режиме добавления (append), чтобы не стирать старые логи.
    m_logFile.open(filePath, std::ios_base::app);
    
    // Крайне важно проверить, удалось ли открыть файл.
    // Если логгер не работает, вся система находится в непредсказуемом состоянии.
    if (!m_logFile.is_open()) {
        // Пробрасываем исключение, которое должно быть обработано на
        // самом верхнем уровне приложения (например, в main.cpp).
        throw std::runtime_error("CRITICAL: Failed to open log file: " + filePath);
    }
}

FileLogger::~FileLogger() {
    // Деструктор std::ofstream автоматически закроет файл,
    // но явный вызов close() считается хорошей практикой.
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void FileLogger::log(const std::string& message) {
    // 1. Получаем текущее системное время
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // 2. Преобразуем его в структуру tm для форматирования.
    // Используем потокобезопасные версии `localtime`, чтобы избежать гонок данных,
    // если приложение станет многопоточным.
    std::tm tm_buf;
#ifdef _WIN32
    // Windows предоставляет _s-версии функций
    localtime_s(&tm_buf, &time_t_now);
#else
    // POSIX-системы (Linux, macOS) предоставляют _r-версии
    localtime_r(&time_t_now, &tm_buf);
#endif

    // 3. Захватываем мьютекс.
    // std::lock_guard - это RAII-обертка, которая автоматически
    // освободит мьютекс при выходе из области видимости, даже если
    // будет брошено исключение. Это делает код безопасным и простым.
    std::lock_guard<std::mutex> lock(m_mutex);

    // 4. Записываем отформатированное время и сообщение в файл.
    // Формат: [ГГГГ-ММ-ДД ЧЧ:ММ:СС] <сообщение>
    if (m_logFile.is_open()) {
        m_logFile << std::put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") 
                  << message 
                  << std::endl; // std::endl добавляет '\n' и сбрасывает буфер файла,
                                // гарантируя немедленную запись на диск.
    }
}
```

---

## <a name="srcinfrastructureFileLoggerh"></a>Файл: `src/infrastructure/FileLogger.h`

```cpp
#pragma once
#include "../core/ILogger.h"
#include <string>
#include <fstream>
#include <mutex> // для потокобезопасности

/**
 * @brief Конкретная реализация ILogger, которая записывает логи в файл.
 *
 * Эта реализация является потокобезопасной, что позволяет использовать
 * один экземпляр логгера из разных частей программы без риска
 * повреждения файла.
 */
class FileLogger : public ILogger {
public:
    /**
     * @brief Конструктор, который открывает файл для логгирования.
     * @param filePath Путь к файлу логов. Файл будет создан, если не существует.
     * @throws std::runtime_error если файл не может быть открыт для записи.
     */
    explicit FileLogger(const std::string& filePath);
    
    /**
     * @brief Деструктор, который корректно закрывает файл логов.
     */
    ~FileLogger();

    // Запрещаем копирование и присваивание, чтобы избежать ситуации,
    // когда два объекта пытаются управлять одним и тем же файловым дескриптором.
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    /**
     * @brief Записывает форматированное сообщение в файл.
     *
     * Сообщение будет содержать временную метку и перенос строки.
     * @param message Текст сообщения для записи.
     */
    void log(const std::string& message) override;

private:
    std::ofstream m_logFile; // Поток для записи в файл
    std::mutex m_mutex;      // Мьютекс для синхронизации доступа к файлу
};
```

---

## <a name="srcinfrastructureFileUserRepositorycpp"></a>Файл: `src/infrastructure/FileUserRepository.cpp`

```cpp
#include "FileUserRepository.h"
#include "../core/User.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>

FileUserRepository::FileUserRepository(const std::string& dbPath) : m_dbPath(dbPath) {
    loadFromFile();
}

void FileUserRepository::loadFromFile() {
    std::ifstream file(m_dbPath);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string username, hash_str, role_str, locked_str, attempts_str, permissions_str;
        if (std::getline(ss, username, ';') &&
            std::getline(ss, hash_str, ';') &&
            std::getline(ss, role_str, ';') &&
            std::getline(ss, locked_str, ';') &&
            std::getline(ss, attempts_str, ';') &&
            std::getline(ss, permissions_str, ';')) { // Чтение нового поля
            try {
                auto user = std::make_shared<User>(
                    username, 
                    std::stoull(hash_str), 
                    (std::stoi(role_str) == 1) ? Role::ADMIN : Role::USER,
                    (std::stoi(locked_str) == 1), 
                    std::stoi(attempts_str),
                    static_cast<unsigned int>(std::stoul(permissions_str)) // Преобразование и передача прав
                );
                m_usersCache[username] = user;
            } catch (const std::exception& e) {
                std::cerr << "Предупреждение: Пропуск поврежденной строки в базе данных пользователей: " << line << std::endl;
            }
        }
    }
}

void FileUserRepository::saveToFile() {
    std::ofstream file(m_dbPath, std::ios_base::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Критическая ошибка: Не удалось сохранить в файл базы данных: " + m_dbPath);
    }
    for (const auto& pair : m_usersCache) {
        const auto& user = pair.second;
        file << user->getUsername() << ";"
             << user->getPasswordHash() << ";"
             << static_cast<int>(user->getRole()) << ";"
             << user->isLocked() << ";"
             << user->getFailedLoginAttempts() << ";"
             << user->getPermissions() << std::endl; // Запись нового поля
    }
}

std::shared_ptr<User> FileUserRepository::findByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_usersCache.find(username);
    return (it != m_usersCache.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<User>> FileUserRepository::getAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<User>> allUsers;
    allUsers.reserve(m_usersCache.size());
    for (const auto& pair : m_usersCache) {
        allUsers.push_back(pair.second);
    }
    return allUsers;
}

void FileUserRepository::add(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.count(user->getUsername())) {
        throw std::runtime_error("Пользователь с таким именем уже существует: " + user->getUsername());
    }
    m_usersCache[user->getUsername()] = user;
    saveToFile();
}

void FileUserRepository::update(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.find(user->getUsername()) == m_usersCache.end()) {
        throw std::runtime_error("Невозможно обновить несуществующего пользователя: " + user->getUsername());
    }
    m_usersCache[user->getUsername()] = user;
    saveToFile();
}

void FileUserRepository::remove(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.erase(username) == 0) {
        throw std::runtime_error("Невозможно удалить несуществующего пользователя: " + username);
    }
    saveToFile();
}
```

---

## <a name="srcinfrastructureFileUserRepositoryh"></a>Файл: `src/infrastructure/FileUserRepository.h`

```cpp
#pragma once
#include "../core/IUserRepository.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class FileUserRepository : public IUserRepository {
public:
    explicit FileUserRepository(const std::string& dbPath);
    ~FileUserRepository() = default;

    FileUserRepository(const FileUserRepository&) = delete;
    FileUserRepository& operator=(const FileUserRepository&) = delete;

    std::shared_ptr<User> findByUsername(const std::string& username) override;
    std::vector<std::shared_ptr<User>> getAll() override;
    void add(std::shared_ptr<User> user) override;
    void update(std::shared_ptr<User> user) override;
    void remove(const std::string& username) override;

private:
    void loadFromFile();
    void saveToFile();
    
    std::string m_dbPath;
    std::map<std::string, std::shared_ptr<User>> m_usersCache;
    std::mutex m_mutex;
};
```

---

## <a name="srcmaincpp"></a>Файл: `src/main.cpp`

```cpp
#include <iostream>
#include <memory>
#include <string>
#include "infrastructure/FileLogger.h"
#include "infrastructure/FileUserRepository.h"
#include "auth/Authenticator.h"
#include "services/UserManager.h"
#include "services/FileManager.h"
#include "cli/CLI.h"
#include "core/SystemSettings.h" 

int main() {
    const std::string LOG_FILE_PATH = "app_activity.log";
    const std::string USER_DATA_PATH = "users.data";
    // --- НАЧАЛО ИЗМЕНЕНИЙ ---
    const std::string SETTINGS_FILE_PATH = "settings.conf";
    // --- КОНЕЦ ИЗМЕНЕНИЙ ---
    
    try {
        // --- НАЧАЛО ИЗМЕНЕНИЙ ---
        SystemSettings settings;
        // Загружаем настройки из файла при запуске
        settings.load(SETTINGS_FILE_PATH);
        // --- КОНЕЦ ИЗМЕНЕНИЙ ---

        FileLogger logger(LOG_FILE_PATH);
        FileUserRepository userRepo(USER_DATA_PATH);

        if (!userRepo.findByUsername("admin")) {
            auto adminUser = std::make_shared<User>("admin", "admin123", Role::ADMIN);
            userRepo.add(adminUser);
            logger.log("Система инициализирована: создан пользователь 'admin' с паролем 'admin123'.");
        }
        
        Authenticator auth(userRepo, logger, settings);
        UserManager userManager(userRepo, logger);
        FileManager fileManager(logger, LOG_FILE_PATH, USER_DATA_PATH);
        
        CLI cli(auth, userManager, fileManager, settings);
        cli.run();

    } catch (const std::exception& e) {
        std::cerr << "\nКРИТИЧЕСКАЯ ОШИБКА: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

---

## <a name="srcservicesFileManagercpp"></a>Файл: `src/services/FileManager.cpp`

```cpp
#include "FileManager.h"
#include "PermissionManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>

// Конструктор, который принимает логгер и пути к защищаемым файлам
FileManager::FileManager(ILogger& logger, const std::string& logFilePath, const std::string& userDbPath) 
    : m_logger(logger), m_logFilePath(logFilePath), m_userDbPath(userDbPath) {}

namespace {
    void ensureDirectoryExists(const std::filesystem::path& path) {
        auto parentDir = path.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }
    }

    bool isSystemFile(const std::string& path_str, const std::string& logFilePath, const std::string& userDbPath) {
        if (!std::filesystem::exists(path_str)) {
            return false;
        }
        try {
            const std::filesystem::path path(path_str);
            if (std::filesystem::exists(logFilePath) && std::filesystem::equivalent(path, logFilePath)) {
                return true;
            }
            if (std::filesystem::exists(userDbPath) && std::filesystem::equivalent(path, userDbPath)) {
                return true;
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ошибка может возникнуть, если файл был удален между exists() и equivalent()
            return false;
        }
        return false;
    }

    void ensureNotSystemFileForUser(const User& actor, const std::string& path, const std::string& logPath, const std::string& dbPath, ILogger& logger) {
        if (actor.getRole() != Role::ADMIN && isSystemFile(path, logPath, dbPath)) {
            logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался получить доступ к системному файлу: " + path);
            throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
        }
    }
}

void FileManager::readFile(const User& actor, const std::string& filePath) {
    PermissionManager::ensure(actor, Permission::READ);
    ensureNotSystemFileForUser(actor, filePath, m_logFilePath, m_userDbPath, m_logger);

    if (!std::filesystem::exists(filePath)) {
        std::ofstream newFile(filePath); 
        if (!newFile.is_open()) {
             throw std::runtime_error("Файл не существует и не может быть создан: " + filePath);
        }
        newFile.close();
        std::cout << "Файл '" << filePath << "' не найден и был создан." << std::endl;
        m_logger.log("Пользователь '" + actor.getUsername() + "' запросил несуществующий файл '" + filePath + "'. Файл создан.");
        return;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для чтения: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::cout << "\n--- Содержимое файла " << filePath << " ---\n" << buffer.str() << "\n--- Конец файла ---\n";
    m_logger.log("Пользователь '" + actor.getUsername() + "' прочитал файл '" + filePath + "'.");
}

void FileManager::writeFile(const User& actor, const std::string& filePath, const std::string& content) {
    PermissionManager::ensure(actor, Permission::WRITE);
    ensureNotSystemFileForUser(actor, filePath, m_logFilePath, m_userDbPath, m_logger);

    try {
        ensureDirectoryExists(filePath);
    } catch (const std::filesystem::filesystem_error& e) {
         throw std::runtime_error("Не удалось создать директорию для файла. " + std::string(e.what()));
    }

    // --- НАЧАЛО ИЗМЕНЕНИЙ ---
    // Открываем файл в режиме дозаписи (append)
    std::ofstream file(filePath, std::ios_base::app);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для записи: " + filePath);
    }

    // Добавляем содержимое и переводим курсор на новую строку
    file << content << std::endl;
    // --- КОНЕЦ ИЗМЕНЕНИЙ ---

    if (!file) {
        throw std::runtime_error("Произошла ошибка во время записи в файл: " + filePath);
    }
    
    // Обновляем сообщение в логе для ясности
    m_logger.log("Пользователь '" + actor.getUsername() + "' дозаписал в файл '" + filePath + "'.");
}

void FileManager::copyFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    ensureNotSystemFileForUser(actor, sourceStr, m_logFilePath, m_userDbPath, m_logger);
    
    std::filesystem::path destPath(destStr);
    if(isSystemFile(destStr, m_logFilePath, m_userDbPath) && actor.getRole() != Role::ADMIN){
         m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался перезаписать системный файл: " + destStr);
         throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
    }


    try {
        std::filesystem::path sourcePath(sourceStr);
        std::filesystem::path finalDestPath = destPath;
        if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
            finalDestPath = destPath / sourcePath.filename();
        }
        ensureDirectoryExists(finalDestPath); 
        std::filesystem::copy(sourcePath, finalDestPath, std::filesystem::copy_options::overwrite_existing);
        m_logger.log("Пользователь '" + actor.getUsername() + "' скопировал файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Операция копирования не удалась. " + std::string(e.what()));
    }
}

void FileManager::moveFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    ensureNotSystemFileForUser(actor, sourceStr, m_logFilePath, m_userDbPath, m_logger);

    std::filesystem::path destPath(destStr);
     if(isSystemFile(destStr, m_logFilePath, m_userDbPath) && actor.getRole() != Role::ADMIN){
         m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался перезаписать системный файл: " + destStr);
         throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
    }
    
    try {
        std::filesystem::path sourcePath(sourceStr);
        std::filesystem::path finalDestPath = destPath;
        if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
            finalDestPath = destPath / sourcePath.filename();
        }
        ensureDirectoryExists(finalDestPath);
        std::filesystem::rename(sourcePath, finalDestPath);
        m_logger.log("Пользователь '" + actor.getUsername() + "' переместил файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Операция перемещения не удалась. " + std::string(e.what()));
    }
}
```

---

## <a name="srcservicesFileManagerh"></a>Файл: `src/services/FileManager.h`

```cpp
#pragma once
#include "../core/ILogger.h"
#include "../core/User.h"
#include <string>

class FileManager {
public:
    // Конструктор теперь принимает пути к двум защищаемым системным файлам
    explicit FileManager(ILogger& logger, const std::string& logFilePath, const std::string& userDbPath);

    void readFile(const User& actor, const std::string& filePath);
    void writeFile(const User& actor, const std::string& filePath, const std::string& content);
    void copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    void moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
private:
    ILogger& m_logger;
    std::string m_logFilePath;
    std::string m_userDbPath; // Путь к файлу с данными пользователей
};
```

---

## <a name="srcservicesPermissionManagerh"></a>Файл: `src/services/PermissionManager.h`

```cpp
#pragma once
#include "../core/User.h"
#include <stdexcept>

// Права определены как битовые флаги (степени двойки) в ПРАВИЛЬНОМ порядке
enum class Permission : unsigned int {
    NONE = 0,
    // Права, назначаемые пользователям
    READ      = 1 << 0, // 1 - Право на чтение
    WRITE     = 1 << 1, // 2 - Право на запись
    COPY_MOVE = 1 << 2, // 4 - Право на копирование и перемещение

    // Права, связанные с ролью (только для администраторов)
    DELETE_USER = 1 << 3,
    CREATE_USER = 1 << 4
};

class PermissionManager {
public:
    static void ensure(const User& user, Permission requiredPermission) {
        if (!has(user, requiredPermission)) {
            throw std::runtime_error("В доступе отказано.");
        }
    }

    static bool has(const User& user, Permission requiredPermission) {
        // Администратор по-прежнему имеет полный доступ ко всему
        if (user.getRole() == Role::ADMIN) {
            return true;
        }

        // Проверяем права на управление пользователями (доступно только админу)
        if (requiredPermission == Permission::CREATE_USER || requiredPermission == Permission::DELETE_USER) {
            return false;
        }

        // Для всех остальных прав (файловые операции) используем битовую маску
        // Побитовое "И" вернет ненулевое значение, только если нужный бит установлен
        return (user.getPermissions() & static_cast<unsigned int>(requiredPermission)) != 0;
    }
};
```

---

## <a name="srcservicesUserManagercpp"></a>Файл: `src/services/UserManager.cpp`

```cpp
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
```

---

## <a name="srcservicesUserManagerh"></a>Файл: `src/services/UserManager.h`

```cpp
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

    // Добавлен параметр permissions для указания прав при создании
    void createUserByAdmin(const User& actor, const std::string& username, const std::string& password, unsigned int permissions);

    void deleteUser(const User& actor, const std::string& usernameToDelete);

    std::vector<std::shared_ptr<User>> listAllUsers(const User& actor);

private:
    IUserRepository& m_userRepository;
    ILogger& m_logger;
};
```

---

## <a name="srcutilsHashcpp"></a>Файл: `src/utils/Hash.cpp`

```cpp
#include "Hash.h"
#include <functional> // для std::hash

size_t hashPassword(const std::string& password) {
    // "Соль" - это случайная строка, добавляемая к паролю перед хешированием.
    // В реальной системе она должна быть уникальной для каждого пользователя.
    // Здесь мы используем статическую соль для упрощения.
    const std::string salt = "a1b2-c3d4-e5f6-a-static-salt-for-a-university-project";
    
    // std::hash - простая хеш-функция, не предназначенная для криптографии.
    return std::hash<std::string>{}(password + salt);
}
```

---

## <a name="srcutilsHashh"></a>Файл: `src/utils/Hash.h`

```cpp
#pragma once
#include <string>

/**
 * @brief Хеширует пароль с использованием стандартной библиотеки.
 * @warning ВАЖНО: Этот метод АБСОЛЮТНО НЕБЕЗОПАСЕН для использования
 * в реальных приложениях. Он создан исключительно для учебных целей,
 * чтобы избежать внешних зависимостей.
 * @param password Пароль в виде открытого текста.
 * @return Хеш пароля.
 */
size_t hashPassword(const std::string& password);
```

---

