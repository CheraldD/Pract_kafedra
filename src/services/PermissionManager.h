#pragma once
#include "../core/User.h"
#include "../core/AppExceptions.h" 
#include <string>                
#include <stdexcept>


enum class Permission : unsigned int {
    NONE = 0,
   
    READ      = 1 << 0, 
    WRITE     = 1 << 1, 
    COPY_MOVE = 1 << 2, 

  
    DELETE_USER = 1 << 3,
    CREATE_USER = 1 << 4
};

class PermissionManager {
public:

    static void ensure(const User& user, Permission requiredPermission, const std::string& actionDescription) {
        if (!has(user, requiredPermission)) {
            std::string logMessage = "Пользователю '" + user.getUsername() + 
                                     "' отказано в доступе при попытке выполнить действие: " + 
                                     actionDescription;
            throw PermissionDeniedException(logMessage);
        }
    }

    static bool has(const User& user, Permission requiredPermission) {
        
        if (user.getRole() == Role::ADMIN) {
            return true;
        }

        
        if (requiredPermission == Permission::CREATE_USER || requiredPermission == Permission::DELETE_USER) {
            return false;
        }
        return (user.getPermissions() & static_cast<unsigned int>(requiredPermission)) != 0;
    }
};