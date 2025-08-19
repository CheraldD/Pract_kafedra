#pragma once
#include "../core/User.h"
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
    static void ensure(const User& user, Permission requiredPermission) {
        if (!has(user, requiredPermission)) {
            throw std::runtime_error("В доступе отказано.");
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