#pragma once
#include <memory>

// Forward declarations
class Authenticator;
class UserManager;
class FileManager;
class User;
struct SystemSettings;
class ILogger; 

class CLI {
public:
    CLI(Authenticator& auth, UserManager& userManager, FileManager& fileManager, SystemSettings& settings, ILogger& logger);
    void run();

private:
    void handleAuthScreen();
    void handleLogin();
    void handleRegistration();
    void handleUserActions();
    void showMainMenu() const;
    void handleSystemSettings();
    Authenticator& m_auth;
    UserManager& m_userManager;
    FileManager& m_fileManager;
    SystemSettings& m_settings; 
    ILogger& m_logger; 
    std::shared_ptr<User> m_currentUser;
    bool m_shouldRun;
};