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
    std::cout << "> " << std::flush;
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

    std::cout << "Имя пользователя: " << std::flush;
    std::getline(std::cin, username);
    
    std::cout << "Пароль: " << std::flush;
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
    
    std::cout << "Введите новое имя пользователя: " << std::flush;
    std::getline(std::cin, username);
    
    std::cout << "Введите пароль (мин. 4 символа): " << std::flush;
    password = getMaskedPassword();

    std::cout << "Подтвердите пароль: " << std::flush;
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
    if (PermissionManager::has(*m_currentUser, Permission::CREATE_USER)) {
        std::cout << "\n║ 6. Создать пользователя (Админ)      ║";
    }
    std::cout << "\n╠═══════════════════════════════════╣"
              << "\n║ 9. Выйти из аккаунта              ║"
              << "\n║ 0. Выйти из приложения            ║"
              << "\n╚═══════════════════════════════════╝" << std::endl;
    std::cout << "> " << std::flush;
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
                case 6: {
                    if (!PermissionManager::has(*m_currentUser, Permission::CREATE_USER)) {
                        std::cout << "\n[✗] Неизвестная команда." << std::endl;
                        break;
                    }

                    std::cout << "\n--- Создание нового пользователя ---" << std::endl;
                    std::string newUsername, newPassword;

                    std::cout << "-> Введите имя нового пользователя (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::getline(std::cin, newUsername);
                    if (newUsername == CANCEL_COMMAND || newUsername.empty()) break;

                    std::cout << "-> Введите пароль (мин. 4 символа): " << std::flush;
                    newPassword = getMaskedPassword();

                    m_userManager.createUserByAdmin(*m_currentUser, newUsername, newPassword);
                    std::cout << "\n[✓] Пользователь '" << newUsername << "' успешно создан." << std::endl;
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