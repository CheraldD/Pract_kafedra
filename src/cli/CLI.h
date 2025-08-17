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