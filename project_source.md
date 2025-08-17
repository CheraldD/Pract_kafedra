# Полный Исходный Код Проекта Системы Управления Доступом
_Сгенерировано: 2025-08-17 18:34:05_

## Содержание
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

## <a name="srcauthAuthenticatorcpp"></a>Файл: `src/auth/Authenticator.cpp`

```cpp
#include "Authenticator.h"
#include "../core/User.h"
#include "../utils/Hash.h"

Authenticator::Authenticator(IUserRepository& repo, ILogger& logger, int maxAttempts)
    : m_userRepository(repo),
      m_logger(logger),
      m_maxFailedAttempts(maxAttempts) {}

std::shared_ptr<User> Authenticator::login(const std::string& username, const std::string& password) {
    auto user = m_userRepository.findByUsername(username);

    if (!user) {
        m_logger.log("Неудачная попытка входа: пользователь '" + username + "' не найден.");
        return nullptr;
    }

    if (user->isLocked()) {
        m_logger.log("Попытка входа в заблокированный аккаунт: '" + username + "'.");
        return nullptr;
    }

    if (user->getPasswordHash() == hashPassword(password)) {
        m_logger.log("Пользователь '" + username + "' успешно вошел в систему.");
        if (user->getFailedLoginAttempts() > 0) {
            user->resetFailedAttempts();
            m_userRepository.update(user);
        }
        return user;
    } else {
        user->incrementFailedAttempts();
        m_logger.log("Неудачная попытка входа для пользователя '" + username + 
                     "'. Попытка " + std::to_string(user->getFailedLoginAttempts()) + 
                     " из " + std::to_string(m_maxFailedAttempts) + ".");

        if (user->getFailedLoginAttempts() >= m_maxFailedAttempts) {
            user->lock();
            m_logger.log("Аккаунт пользователя '" + username + "' заблокирован из-за большого количества неудачных попыток входа.");
        }
        
        m_userRepository.update(user);
        return nullptr;
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
    /**
     * @param repo Репозиторий для доступа к данным пользователей.
     * @param logger Логгер для записи событий.
     * @param maxAttempts Максимальное число неудачных попыток входа до блокировки.
     */
    Authenticator(IUserRepository& repo, ILogger& logger, int maxAttempts = 3);

    /**
     * @brief Выполняет попытку входа пользователя в систему.
     * @param username Имя пользователя.
     * @param password Пароль в открытом виде.
     * @return Умный указатель на объект User в случае успеха, иначе nullptr.
     */
    std::shared_ptr<User> login(const std::string& username, const std::string& password);

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

#include "../auth/Authenticator.h"
#include "../services/UserManager.h"
#include "../services/FileManager.h"
#include "../services/PermissionManager.h"
#include "../core/User.h"

namespace {
    void clearInputBuffer() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

CLI::CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager)
    : m_auth(auth), 
      m_userManager(userManager),
      m_fileManager(fileManager),
      m_currentUser(nullptr) {}

void CLI::run() {
    while (true) {
        if (!m_currentUser) {
            handleAuthScreen();
        }
        
        if (m_currentUser) {
            handleUserActions();
        } else {
            // Если пользователь выбрал выход на экране аутентификации
            break;
        }
    }
    std::cout << "\nЗавершение работы. До свидания!" << std::endl;
}

void CLI::handleAuthScreen() {
    std::cout << "\n--- Система Управления Доступом ---" << std::endl;
    std::cout << "1. Вход" << std::endl;
    std::cout << "2. Регистрация" << std::endl;
    std::cout << "0. Выход" << std::endl;
    std::cout << "> ";

    int choice;
    std::cin >> choice;

    if (std::cin.fail()) {
        std::cout << "Некорректный ввод. Пожалуйста, введите число." << std::endl;
        std::cin.clear();
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    switch (choice) {
        case 1:
            handleLogin();
            break;
        case 2:
            handleRegistration();
            break;
        case 0:
            // m_currentUser останется nullptr, что приведет к выходу из главного цикла
            break;
        default:
            std::cout << "Неизвестная команда." << std::endl;
            break;
    }
}

void CLI::handleLogin() {
    std::cout << "\n--- Вход в систему ---" << std::endl;
    std::string username, password;

    std::cout << "Имя пользователя: ";
    std::cin >> username;
    
    std::cout << "Пароль: ";
    std::cin >> password;
    
    m_currentUser = m_auth.login(username, password);
    
    if (!m_currentUser) {
        std::cout << "Ошибка входа. Неверные учетные данные или аккаунт заблокирован.\n" << std::endl;
    } else {
        std::cout << "Добро пожаловать, " << m_currentUser->getUsername() << "!" << std::endl;
    }
}

void CLI::handleRegistration() {
    std::cout << "\n--- Регистрация нового пользователя ---" << std::endl;
    std::string username, password, passwordConfirm;
    
    std::cout << "Введите новое имя пользователя: ";
    std::cin >> username;
    
    std::cout << "Введите пароль (мин. 4 символа): ";
    std::cin >> password;

    std::cout << "Подтвердите пароль: ";
    std::cin >> passwordConfirm;

    if (password != passwordConfirm) {
        std::cout << "Ошибка: пароли не совпадают." << std::endl;
        return;
    }

    try {
        m_userManager.createUser(username, password);
        std::cout << "Пользователь '" << username << "' успешно зарегистрирован. Теперь вы можете войти." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка регистрации: " << e.what() << std::endl;
    }
}

void CLI::showMainMenu() const {
    std::cout << "\n--- Главное меню (Пользователь: " << m_currentUser->getUsername() << ") ---" << std::endl;
    std::cout << "1. Прочитать файл" << std::endl;
    std::cout << "2. Записать в файл" << std::endl;
    std::cout << "3. Копировать файл" << std::endl;
    std::cout << "4. Переместить файл" << std::endl;
    if (PermissionManager::has(*m_currentUser, Permission::DELETE_USER)) {
        std::cout << "5. Удалить пользователя (только для Администратора)" << std::endl;
    }
    std::cout << "9. Выйти из аккаунта" << std::endl;
    std::cout << "0. Выйти из приложения" << std::endl;
    std::cout << "> ";
}

void CLI::handleUserActions() {
    int choice = -1;
    while (m_currentUser) {
        showMainMenu();
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cout << "Некорректный ввод. Пожалуйста, введите число." << std::endl;
            std::cin.clear();
            clearInputBuffer();
            choice = -1;
            continue;
        }
        
        clearInputBuffer();

        std::string path1, path2, content, usernameToDelete;

        try {
            switch (choice) {
                case 1:
                    std::cout << "Введите путь к файлу для чтения: ";
                    std::getline(std::cin, path1);
                    m_fileManager.readFile(*m_currentUser, path1);
                    break;
                case 2:
                    std::cout << "Введите путь к файлу для записи: ";
                    std::getline(std::cin, path1);
                    std::cout << "Введите содержимое (одной строкой): ";
                    std::getline(std::cin, content);
                    m_fileManager.writeFile(*m_currentUser, path1, content);
                    break;
                case 3:
                    std::cout << "Введите путь к исходному файлу: ";
                    std::getline(std::cin, path1);
                    std::cout << "Введите путь к файлу назначения: ";
                    std::getline(std::cin, path2);
                    m_fileManager.copyFile(*m_currentUser, path1, path2);
                    break;
                case 4:
                    std::cout << "Введите путь к исходному файлу: ";
                    std::getline(std::cin, path1);
                    std::cout << "Введите путь к файлу назначения: ";
                    std::getline(std::cin, path2);
                    m_fileManager.moveFile(*m_currentUser, path1, path2);
                    break;
                case 5:
                    if (PermissionManager::has(*m_currentUser, Permission::DELETE_USER)) {
                        std::cout << "Введите имя пользователя для удаления: ";
                        std::getline(std::cin, usernameToDelete);
                        m_userManager.deleteUser(*m_currentUser, usernameToDelete);
                        std::cout << "Пользователь '" << usernameToDelete << "' успешно удален." << std::endl;
                    } else { std::cout << "Неизвестная команда." << std::endl; }
                    break;
                case 9:
                    m_currentUser = nullptr;
                    std::cout << "Вы вышли из системы." << std::endl;
                    break;
                case 0:
                    m_currentUser = nullptr;
                    // Этот return приведет к выходу из цикла в run(), так как m_currentUser == nullptr
                    return;
                default:
                    std::cout << "Неизвестная команда. Попробуйте еще раз." << std::endl;
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Операция не удалась: " << e.what() << std::endl;
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
    // Новые приватные методы для UI
    void handleAuthScreen();
    void handleLogin();
    void handleRegistration();
    void handleUserActions();
    void showMainMenu() const;

    Authenticator& m_auth;
    UserManager& m_userManager;
    FileManager& m_fileManager;
    std::shared_ptr<User> m_currentUser;
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
#include <memory> // для std::shared_ptr

// Предварительное объявление, чтобы избежать включения полного заголовка User.h,
// если это не нужно, и ускорить компиляцию.
class User; 

/**
 * @brief Абстрактный интерфейс для репозитория пользователей.
 * Определяет контракт для всех операций по доступу к данным пользователей.
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    // Использование умных указателей (shared_ptr) - хорошая практика для управления
    // временем жизни объектов, передаваемых между модулями.
    
    /**
     * @brief Находит пользователя по его имени.
     * @return Указатель на пользователя, если найден, иначе nullptr.
     */
    virtual std::shared_ptr<User> findByUsername(const std::string& username) = 0;

    /**
     * @brief Добавляет нового пользователя в хранилище.
     */
    virtual void add(std::shared_ptr<User> user) = 0;
    
    /**
     * @brief Обновляет данные существующего пользователя в хранилище.
     */
    virtual void update(std::shared_ptr<User> user) = 0;

    /**
     * @brief Удаляет пользователя из хранилища по его имени.
     */
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

// (Этот файл практически не изменился, только сообщения об ошибках)

FileUserRepository::FileUserRepository(const std::string& dbPath) : m_dbPath(dbPath) {
    loadFromFile();
}

void FileUserRepository::loadFromFile() {
    // ... реализация без изменений
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

class FileUserRepository : public IUserRepository {
public:
    explicit FileUserRepository(const std::string& dbPath);
    ~FileUserRepository() = default;

    FileUserRepository(const FileUserRepository&) = delete;
    FileUserRepository& operator=(const FileUserRepository&) = delete;

    std::shared_ptr<User> findByUsername(const std::string& username) override;
    void add(std::shared_ptr<User> user) override;
    void update(std::shared_ptr<User> user) override;
    void remove(const std::string& username) override;

private:
    void loadFromFile();
    void saveToFile();
    
    std::string m_dbPath;
    // Кэш для быстрого доступа. Ключ - username.
    std::map<std::string, std::shared_ptr<User>> m_usersCache;
    // Мьютекс для защиты кэша и файла от одновременного доступа
    std::mutex m_mutex;
};
```

---

## <a name="srcmaincpp"></a>Файл: `src/main.cpp`

```cpp
#include <iostream>
#include <memory>
#include "infrastructure/FileLogger.h"
#include "infrastructure/FileUserRepository.h"
#include "auth/Authenticator.h"
#include "services/UserManager.h"
#include "services/FileManager.h"
#include "cli/CLI.h"


int main() {
    
    try {
        FileLogger logger("app_activity.log");
        FileUserRepository userRepo("users.data");

        if (!userRepo.findByUsername("admin")) {
            auto adminUser = std::make_shared<User>("admin", "admin123", Role::ADMIN);
            userRepo.add(adminUser);
            logger.log("Система инициализирована: создан пользователь 'admin' с паролем 'admin123'.");
        }
        
        Authenticator auth(userRepo, logger);
        UserManager userManager(userRepo, logger);
        FileManager fileManager(logger);
        
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

FileManager::FileManager(ILogger& logger) : m_logger(logger) {}

void FileManager::readFile(const User& actor, const std::string& filePath) {
    PermissionManager::ensure(actor, Permission::READ);
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог прочитать файл '" + filePath + "'. Причина: Файл не найден или отказано в доступе ОС.");
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

    std::ofstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Не удалось открыть файл.");
        throw std::runtime_error("Не удалось открыть файл для записи: " + filePath);
    }

    file << content;
    
    if (!file) {
         m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Произошла ошибка во время записи.");
        throw std::runtime_error("Произошла ошибка во время записи в файл: " + filePath);
    }
    
    m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' записал " + std::to_string(content.length()) + " байт в файл '" + filePath + "'.");
}

void FileManager::copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    try {
        const auto options = std::filesystem::copy_options::overwrite_existing;
        std::filesystem::copy(sourcePath, destPath, options);
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' скопировал файл из '" + sourcePath + "' в '" + destPath + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог скопировать файл из '" + sourcePath + "'. Причина: " + e.what());
        throw std::runtime_error("Операция копирования файла не удалась: " + std::string(e.what()));
    }
}

void FileManager::moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    try {
        std::filesystem::rename(sourcePath, destPath);
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' переместил файл из '" + sourcePath + "' в '" + destPath + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог переместить файл из '" + sourcePath + "'. Причина: " + e.what());
        throw std::runtime_error("Операция перемещения файла не удалась: " + std::string(e.what()));
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

/**
 * @brief Предоставляет полнофункциональные операции с файловой системой.
 * Все операции проверяют права доступа и логируют результат.
 */
class FileManager {
public:
    explicit FileManager(ILogger& logger);

    /**
     * @brief Читает содержимое файла и выводит его в консоль.
     * @throws std::runtime_error, если файл не может быть открыт.
     */
    void readFile(const User& actor, const std::string& filePath);

    /**
     * @brief Записывает (перезаписывая) контент в файл.
     * @param content Содержимое для записи в файл.
     * @throws std::runtime_error, если файл не может быть открыт для записи.
     */
    void writeFile(const User& actor, const std::string& filePath, const std::string& content);

    /**
     * @brief Копирует файл из одного места в другое.
     * @throws std::runtime_error, если операция не удалась.
     */
    void copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
    /**
     * @brief Перемещает (или переименовывает) файл.
     * @throws std::runtime_error, если операция не удалась.
     */
    void moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
private:
    ILogger& m_logger;
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

UserManager::UserManager(IUserRepository& repo, ILogger& logger)
    : m_userRepository(repo), m_logger(logger) {}

void UserManager::createUser(const std::string& username, const std::string& password) {
    // 1. Валидация
    if (username.length() < 3) {
        throw std::runtime_error("Имя пользователя должно быть не менее 3 символов.");
    }
    if (password.length() < 4) {
        throw std::runtime_error("Пароль должен быть не менее 4 символов.");
    }
    if (m_userRepository.findByUsername(username)) {
        throw std::runtime_error("Пользователь с таким именем уже существует.");
    }

    // 2. Создание и добавление
    auto newUser = std::make_shared<User>(username, password, Role::USER);
    m_userRepository.add(newUser);

    // 3. Логирование
    m_logger.log("Создан новый пользователь: '" + username + "'.");
}

void UserManager::deleteUser(const User& actor, const std::string& usernameToDelete) {
    PermissionManager::ensure(actor, Permission::DELETE_USER);
    
    if (actor.getUsername() == usernameToDelete) {
        throw std::runtime_error("Вы не можете удалить свой собственный аккаунт.");
    }
    
    // Проверяем, существует ли пользователь, перед удалением
    if (!m_userRepository.findByUsername(usernameToDelete)) {
        throw std::runtime_error("Пользователь с именем '" + usernameToDelete + "' не найден.");
    }

    m_userRepository.remove(usernameToDelete);
    
    m_logger.log("Пользователь '" + actor.getUsername() + "' удалил пользователя '" + usernameToDelete + "'.");
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

class UserManager {
public:
    UserManager(IUserRepository& repo, ILogger& logger);

    /**
     * @brief Создает нового пользователя с ролью USER.
     * @throws std::runtime_error если имя пользователя уже занято или данные некорректны.
     */
    void createUser(const std::string& username, const std::string& password);

    /**
     * @brief Удаляет пользователя.
     * @param actor Пользователь, выполняющий действие.
     * @param usernameToDelete Имя пользователя, которого нужно удалить.
     */
    void deleteUser(const User& actor, const std::string& usernameToDelete);

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

