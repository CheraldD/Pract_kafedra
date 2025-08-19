#include <iostream>
#include <memory>
#include <string>
#include "infrastructure/FileLogger.h"
#include "infrastructure/FileUserRepository.h"
#include "auth/Authenticator.h"
#include "services/UserManager.h"
#include "services/FileManager.h"
#include "cli/CLI.h"
#include "core/SystemSettings.h" // Подключаем новый заголовок

int main() {
    const std::string LOG_FILE_PATH = "app_activity.log";
    const std::string USER_DATA_PATH = "users.data";
    
    try {
        SystemSettings settings;

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