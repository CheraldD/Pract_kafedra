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

// ... handleAuthScreen, handleLogin, handleRegistration, showMainMenu, handleSystemSettings (без изменений)

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
                case 1: {
                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к файлу для чтения (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;
                    m_fileManager.readFile(*m_currentUser, path);
                    break;
                }
                case 2: {
                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите путь к файлу для записи (или '" << CANCEL_COMMAND << "'): " << std::flush;
                    std::string path;
                    std::getline(std::cin, path);
                    if (path == CANCEL_COMMAND || path.empty()) break;

                    std::cout << Color::CYAN << "-> " << Color::RESET << "Введите содержимое (одной строкой): " << std::flush;
                    std::string content;
                    std::getline(std::cin, content);
                    m_fileManager.writeFile(*m_currentUser, path, content);
                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Запись в файл '" << path << "' успешно завершена." << std::endl;
                    break;
                }
                case 3: case 4: {
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
                case 5: {
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
                case 6: {
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

                    // --- НАЧАЛО ИЗМЕНЕНИЙ ---
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
                    
                    unsigned int permissions = 0;
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

                    if (permissions == 0) {
                        break; 
                    }
                    
                    // Вызов обновленного метода с передачей маски прав
                    m_userManager.createUserByAdmin(*m_currentUser, newUsername, newPassword, permissions);
                    // --- КОНЕЦ ИЗМЕНЕНИЙ ---

                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Пользователь '" << newUsername << "' успешно создан." << std::endl;
                    break;
                }
                case 7: {
                     if (m_currentUser->getRole() != Role::ADMIN) {
                        std::cout << "\n" << Color::RED << "[✗] " << Color::RESET << "Неизвестная команда." << std::endl; break;
                    }
                    handleSystemSettings();
                    break;
                }
                case 9:
                    m_currentUser = nullptr;
                    std::cout << "\n" << Color::BRIGHT_GREEN << "[✓] " << Color::RESET << "Вы вышли из системы." << std::endl;
                    break;
                case 0:
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