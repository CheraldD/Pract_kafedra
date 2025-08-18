#pragma once
#include "../core/User.h"
#include <stdexcept>

enum class Permission {
    READ,
    WRITE,
    COPY_MOVE,
    DELETE_USER,
    CREATE_USER
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
                case Permission::CREATE_USER:
                    return false;
            }
        }

        return false;
    }
};