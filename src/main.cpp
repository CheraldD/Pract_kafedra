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