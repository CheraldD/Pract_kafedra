# Полный Исходный Код Проекта Системы Управления Доступом
_Сгенерировано: 2025-08-18 23:55:20_

## Содержание
- [`src/auth/AuthExceptions.h`](#srcauthAuthExceptionsh)
- [`src/auth/Authenticator.cpp`](#srcauthAuthenticatorcpp)
- [`src/auth/Authenticator.h`](#srcauthAuthenticatorh)
- [`src/cli/CLI.cpp`](#srccliCLIcpp)
- [`src/cli/CLI.h`](#srccliCLIh)
- [`src/core/ILogger.h`](#srccoreILoggerh)
- [`src/core/IUserRepository.h`](#srccoreIUserRepositoryh)
- [`src/core/Role.h`](#srccoreRoleh)
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
```

---

## <a name="srcauthAuthenticatorh"></a>Файл: `src/auth/Authenticator.h`

```cpp
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
```

---

## <a name="srccliCLIcpp"></a>Файл: `src/cli/CLI.cpp`

```cpp
#include "CLI.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// Платформо-зависимые заголовочные файлы
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include "../auth/Authenticator.h"
#include "../auth/AuthExceptions.h"
#include "../services/UserManager.h"
#include "../services/FileManager.h"
#include "../services/PermissionManager.h"
#include "../core/User.h"

namespace {
    const std::string CANCEL_COMMAND = "cancel";

    void clearInputBuffer() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string getMaskedPassword() {
        std::string password;
        char ch;

#ifdef _WIN32
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b";
                }
            } else {
                password += ch;
                std::cout << '*';
            }
        }
#else
        termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n') {
            if (ch == 127 || ch == 8) {
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
#endif
        std::cout << std::endl;
        return password;
    }
}

CLI::CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager)
    : m_auth(auth),
      m_userManager(userManager),
      m_fileManager(fileManager),
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
    std::cout << "\n[✓] Завершение работы. До свидания!" << std::endl;
}

void CLI::handleAuthScreen() {
    std::cout << "\n╔═══════════════════════════════════╗"
              << "\n║    Система Управления Доступом    ║"
              << "\n╠═══════════════════════════════════╣"
              << "\n║ 1. Вход                           ║"
              << "\n║ 2. Регистрация                    ║"
              << "\n║ 0. Выход                          ║"
              << "\n╚═══════════════════════════════════╝" << std::endl;
    std::cout << "> " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    int choice;
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cout << "\n[✗] Некорректный ввод. Пожалуйста, введите число." << std::endl;
        std::cin.clear();
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    switch (choice) {
        case 1: handleLogin(); break;
        case 2: handleRegistration(); break;
        case 0: m_shouldRun = false; break;
        default: std::cout << "\n[✗] Неизвестная команда." << std::endl; break;
    }
}

void CLI::handleLogin() {
    std::cout << "\n--- Вход в систему ---" << std::endl;
    std::string username, password;

    std::cout << "Имя пользователя: " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    std::getline(std::cin, username);
    
    std::cout << "Пароль: " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    password = getMaskedPassword(); 
    
    try {
        m_currentUser = m_auth.login(username, password);
        std::cout << "\n[✓] Добро пожаловать, " << m_currentUser->getUsername() << "!" << std::endl;
    } 
    catch (const AuthenticationException& e) {
        std::cout << "\n[✗] Ошибка входа: " << e.what() << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[✗] Произошла непредвиденная системная ошибка: " << e.what() << std::endl;
    }
}

void CLI::handleRegistration() {
    std::cout << "\n--- Регистрация нового пользователя ---" << std::endl;
    std::string username, password, passwordConfirm;
    
    std::cout << "Введите новое имя пользователя: " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    std::getline(std::cin, username);
    
    std::cout << "Введите пароль (мин. 4 символа): " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    password = getMaskedPassword();

    std::cout << "Подтвердите пароль: " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
    passwordConfirm = getMaskedPassword();

    if (password != passwordConfirm) {
        std::cout << "\n[✗] Ошибка: пароли не совпадают." << std::endl;
        return;
    }

    try {
        m_userManager.createUser(username, password);
        std::cout << "\n[✓] Пользователь '" << username << "' успешно зарегистрирован. Теперь вы можете войти." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[✗] Ошибка регистрации: " << e.what() << std::endl;
    }
}

void CLI::showMainMenu() const {
    std::cout << "\n╔═══════════════════════════════════╗"
              << "\n║ Меню (Пользователь: " << m_currentUser->getUsername() << ")"
              << "\n╠═══════════════════════════════════╣"
              << "\n║ 1. Прочитать файл                 ║"
              << "\n║ 2. Записать в файл                ║"
              << "\n║ 3. Копировать файл                ║"
              << "\n║ 4. Переместить файл               ║";
    if (PermissionManager::has(*m_currentUser, Permission::DELETE_USER)) {
        std::cout << "\n║ 5. Удалить пользователя (Админ)     ║";
    }
    std::cout << "\n╠═══════════════════════════════════╣"
              << "\n║ 9. Выйти из аккаунта              ║"
              << "\n║ 0. Выйти из приложения            ║"
              << "\n╚═══════════════════════════════════╝" << std::endl;
    std::cout << "> " << std::flush; // ИЗМЕНЕНИЕ: Добавлен std::flush
}

void CLI::handleUserActions() {
    int choice = -1;
    while (m_currentUser && m_shouldRun) {
        showMainMenu();
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cout << "\n[✗] Некорректный ввод. Пожалуйста, введите число." << std::endl;
            std::cin.clear();
            clearInputBuffer();
            choice = -1;
            continue;
        }
        
        clearInputBuffer();

        try {
            switch (choice) {
                case 1: {
                    std::cout << "-> Введите путь к файлу для чтения (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;
                    m_fileManager.readFile(*m_currentUser, path);
                    break;
                }
                case 2: {
                    std::cout << "-> Введите путь к файлу для записи (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;

                    std::cout << "-> Введите содержимое (одной строкой): " << std::flush;
                    std::string content;
                    std::getline(std::cin, content);
                    m_fileManager.writeFile(*m_currentUser, path, content);
                    std::cout << "\n[✓] Запись в файл '" << path << "' успешно завершена." << std::endl;
                    break;
                }
                case 3: {
                    std::cout << "-> Введите путь к исходному файлу (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string source;
                    std::getline(std::cin, source);
                    if (source == CANCEL_COMMAND || source.empty()) break;

                    std::cout << "-> Введите путь к файлу/папке назначения: " << std::flush;
                    std::string dest;
                    std::getline(std::cin, dest);
                    if (dest == CANCEL_COMMAND || dest.empty()) break;

                    m_fileManager.copyFile(*m_currentUser, source, dest);
                    std::cout << "\n[✓] Файл '" << source << "' успешно скопирован." << std::endl;
                    break;
                }
                case 4: {
                    std::cout << "-> Введите путь к исходному файлу (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string source;
                    std::getline(std::cin, source);
                    if (source == CANCEL_COMMAND || source.empty()) break;

                    std::cout << "-> Введите путь к файлу/папке назначения: " << std::flush;
                    std::string dest;
                    std::getline(std::cin, dest);
                    if (dest == CANCEL_COMMAND || dest.empty()) break;

                    m_fileManager.moveFile(*m_currentUser, source, dest);
                    std::cout << "\n[✓] Файл '" << source << "' успешно перемещен." << std::endl;
                    break;
                }
                case 5: {
                    if (!PermissionManager::has(*m_currentUser, Permission::DELETE_USER)) {
                        std::cout << "\n[✗] Неизвестная команда." << std::endl;
                        break;
                    }
                    std::cout << "\n--- Список Пользователей ---" << std::endl;
                    const auto users = m_userManager.listAllUsers(*m_currentUser);
                    for (const auto& user : users) {
                        std::cout << "- " << user->getUsername()
                                  << " (Роль: " << (user->getRole() == Role::ADMIN ? "Админ" : "Пользователь") << ")"
                                  << (user->isLocked() ? " [ЗАБЛОКИРОВАН]" : "") << std::endl;
                    }
                    std::cout << "--------------------------\n";

                    std::cout << "-> Введите имя пользователя для удаления (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string usernameToDelete;
                    std::getline(std::cin, usernameToDelete);
                    if (usernameToDelete == CANCEL_COMMAND || usernameToDelete.empty()) break;

                    m_userManager.deleteUser(*m_currentUser, usernameToDelete);
                    std::cout << "\n[✓] Пользователь '" << usernameToDelete << "' успешно удален." << std::endl;
                    break;
                }
                case 9:
                    m_currentUser = nullptr;
                    std::cout << "\n[✓] Вы вышли из системы." << std::endl;
                    break;
                case 0:
                    m_shouldRun = false;
                    m_currentUser = nullptr;
                    break;
                default:
                    std::cout << "\n[✗] Неизвестная команда. Попробуйте еще раз." << std::endl;
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "\n[✗] Операция не удалась: " << e.what() << std::endl;
        }
    }
}
```

---

## <a name="srccliCLIh"></a>Файл: `src/cli/CLI.h`

```cpp
#pragma once
#include <memory>

class Authenticator;
class UserManager;
class FileManager;
class User;

class CLI {
public:
    CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager);
    void run();

private:
    void handleAuthScreen();
    void handleLogin();
    void handleRegistration();
    void handleUserActions();
    void showMainMenu() const;

    Authenticator& m_auth;
    UserManager& m_userManager;
    FileManager& m_fileManager;
    std::shared_ptr<User> m_currentUser;
    bool m_shouldRun; // Флаг для контроля основного цикла программы
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

## <a name="srccoreUsercpp"></a>Файл: `src/core/User.cpp`

```cpp
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
        std::string username, hash_str, role_str, locked_str, attempts_str;
        if (std::getline(ss, username, ';') &&
            std::getline(ss, hash_str, ';') &&
            std::getline(ss, role_str, ';') &&
            std::getline(ss, locked_str, ';') &&
            std::getline(ss, attempts_str, ';')) {
            try {
                auto user = std::make_shared<User>(
                    username, 
                    std::stoull(hash_str), 
                    (std::stoi(role_str) == 1) ? Role::ADMIN : Role::USER,
                    (std::stoi(locked_str) == 1), 
                    std::stoi(attempts_str)
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
             << user->getFailedLoginAttempts() << std::endl;
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


int main() {
    const std::string LOG_FILE_PATH = "app_activity.log";
    
    try {
        FileLogger logger(LOG_FILE_PATH);
        FileUserRepository userRepo("users.data");

        if (!userRepo.findByUsername("admin")) {
            auto adminUser = std::make_shared<User>("admin", "admin123", Role::ADMIN);
            userRepo.add(adminUser);
            logger.log("Система инициализирована: создан пользователь 'admin' с паролем 'admin123'.");
        }
        
        Authenticator auth(userRepo, logger);
        UserManager userManager(userRepo, logger);
        FileManager fileManager(logger, LOG_FILE_PATH);
        
        CLI cli(auth, userManager, fileManager);
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

// Конструктор, который принимает логгер и путь к файлу журнала для его защиты
FileManager::FileManager(ILogger& logger, const std::string& logFilePath) 
    : m_logger(logger), m_logFilePath(logFilePath) {}

namespace {
    /**
     * @brief ВАЖНАЯ ФУНКЦИЯ: Создает родительские директории для указанного пути.
     * 
     * Проверяет, существует ли родительский каталог для файла,
     * и если нет, рекурсивно создает всю необходимую структуру.
     * @param path Путь к конечному файлу.
     */
    void ensureDirectoryExists(const std::filesystem::path& path) {
        auto parentDir = path.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            // Эта команда создает все несуществующие папки в пути
            std::filesystem::create_directories(parentDir);
        }
    }
}

void FileManager::readFile(const User& actor, const std::string& filePath) {
    PermissionManager::ensure(actor, Permission::READ);

    if (std::filesystem::exists(filePath) && std::filesystem::equivalent(filePath, m_logFilePath)) {
        if (actor.getRole() != Role::ADMIN) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался прочитать файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала разрешен только администраторам.");
        }
    }

    // Если файл не существует, он создается
    if (!std::filesystem::exists(filePath)) {
        std::ofstream newFile(filePath); 
        if (!newFile.is_open()) {
             m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' пытался прочитать несуществующий файл '" + filePath + "', но создать его не удалось.");
             throw std::runtime_error("Файл не существует и не может быть создан: " + filePath + ". Проверьте права доступа.");
        }
        newFile.close();
        std::cout << "Файл '" << filePath << "' не найден и был создан." << std::endl;
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' запросил чтение несуществующего файла '" + filePath + "'. Файл создан.");
        return;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог прочитать файл '" + filePath + "'. Причина: Отказано в доступе ОС.");
        throw std::runtime_error("Не удалось открыть файл для чтения: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    
    std::cout << "\n--- Содержимое файла " << filePath << " ---\n"
              << buffer.str()
              << "\n--- Конец файла ---\n";

    m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' прочитал файл '" + filePath + "'.");
}

void FileManager::writeFile(const User& actor, const std::string& filePath, const std::string& content) {
    PermissionManager::ensure(actor, Permission::WRITE);

    if (std::filesystem::exists(filePath) && std::filesystem::equivalent(filePath, m_logFilePath)) {
        if (actor.getRole() != Role::ADMIN) {
             m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался записать в файл журнала.");
             throw std::runtime_error("Доступ к файлу журнала разрешен только администраторам.");
        }
    }

    try {
        // Убеждаемся, что директория для файла существует (создаем, если нужно)
        ensureDirectoryExists(filePath);
    } catch (const std::filesystem::filesystem_error& e) {
         m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: не удалось создать директорию назначения. " + e.what());
         throw std::runtime_error("Операция записи не удалась. Убедитесь, что у вас есть права на запись в эту директорию. Системная ошибка: " + std::string(e.what()));
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Не удалось открыть файл (возможно, нет прав доступа).");
        throw std::runtime_error("Не удалось открыть файл для записи: " + filePath + ". Проверьте права доступа.");
    }

    file << content;
    
    if (!file) {
         m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Произошла ошибка во время записи.");
        throw std::runtime_error("Произошла ошибка во время записи в файл: " + filePath);
    }
    
    m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' записал " + std::to_string(content.length()) + " байт в файл '" + filePath + "'.");
}

void FileManager::copyFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);

    if (actor.getRole() != Role::ADMIN) {
        if ((std::filesystem::exists(sourceStr) && std::filesystem::equivalent(sourceStr, m_logFilePath)) ||
            (std::filesystem::exists(destStr) && std::filesystem::equivalent(destStr, m_logFilePath))) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался скопировать файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала запрещен.");
        }
    }

    std::filesystem::path sourcePath(sourceStr);
    std::filesystem::path destPath(destStr);
    std::filesystem::path finalDestPath = destPath;

    // Если путь назначения - существующая папка, копируем файл внутрь
    if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
        finalDestPath = destPath / sourcePath.filename();
    }

    try {
        // **КЛЮЧЕВОЙ МОМЕНТ**: Гарантируем, что родительская директория для
        // конечного файла существует. Если нет — она будет создана.
        ensureDirectoryExists(finalDestPath); 
        
        const auto options = std::filesystem::copy_options::overwrite_existing;
        std::filesystem::copy(sourcePath, finalDestPath, options);
        
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' скопировал файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог скопировать файл из '" + sourceStr + "'. Причина: " + e.what());
        throw std::runtime_error("Операция копирования не удалась. Проверьте путь и права доступа. Системная ошибка: " + std::string(e.what()));
    }
}

void FileManager::moveFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);

    if (actor.getRole() != Role::ADMIN) {
       if ((std::filesystem::exists(sourceStr) && std::filesystem::equivalent(sourceStr, m_logFilePath)) ||
           (std::filesystem::exists(destStr) && std::filesystem::equivalent(destStr, m_logFilePath))) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался переместить файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала запрещен.");
        }
    }

    std::filesystem::path sourcePath(sourceStr);
    std::filesystem::path destPath(destStr);
    std::filesystem::path finalDestPath = destPath;
    
    // Если путь назначения - существующая папка, перемещаем файл внутрь
    if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
        finalDestPath = destPath / sourcePath.filename();
    }

    try {
        // **КЛЮЧЕВОЙ МОМЕНТ**: Гарантируем, что родительская директория для
        // конечного файла существует. Если нет — она будет создана.
        ensureDirectoryExists(finalDestPath);
        
        std::filesystem::rename(sourcePath, finalDestPath);

        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' переместил файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог переместить файл из '" + sourceStr + "'. Причина: " + e.what());
        throw std::runtime_error("Операция перемещения не удалась. Проверьте путь и права доступа. Системная ошибка: " + std::string(e.what()));
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
    explicit FileManager(ILogger& logger, const std::string& logFilePath);

    /**
     * @brief Читает содержимое файла и выводит его в консоль.
     * Если файл не существует, создает его.
     */
    void readFile(const User& actor, const std::string& filePath);

    void writeFile(const User& actor, const std::string& filePath, const std::string& content);

    /**
     * @brief Копирует файл. Создает директорию назначения, если она не существует.
     */
    void copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
    /**
     * @brief Перемещает файл. Создает директорию назначения, если она не существует.
     */
    void moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
private:
    ILogger& m_logger;
    std::string m_logFilePath;
};
```

---

## <a name="srcservicesPermissionManagerh"></a>Файл: `src/services/PermissionManager.h`

```cpp
#pragma once
#include "../core/User.h"
#include <stdexcept>

enum class Permission {
    READ,
    WRITE,
    COPY_MOVE,
    DELETE_USER
};

class PermissionManager {
public:
    static void ensure(const User& user, Permission requiredPermission) {
        if (!has(user, requiredPermission)) {
            throw std::runtime_error("В доступе отказано.");
        }
    }

    static bool has(const User& user, Permission requiredPermission) {
        if (user.getRole() == Role::ADMIN) {
            return true;
        }

        if (user.getRole() == Role::USER) {
            switch (requiredPermission) {
                case Permission::READ:
                case Permission::WRITE:
                case Permission::COPY_MOVE:
                    return true;
                case Permission::DELETE_USER:
                    return false;
            }
        }

        return false;
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

    auto newUser = std::make_shared<User>(username, password, Role::USER);
    m_userRepository.add(newUser);

    m_logger.log("Создан новый пользователь: '" + username + "'.");
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
    // Для получения списка пользователей требуются те же права, что и для удаления
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

    void deleteUser(const User& actor, const std::string& usernameToDelete);

    /**
     * @brief Возвращает список всех пользователей.
     * @param actor Пользователь, выполняющий действие (для проверки прав).
     * @return Вектор с указателями на пользователей.
     * @throws std::runtime_error если у пользователя нет прав на просмотр списка.
     */
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

