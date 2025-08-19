#pragma once
#include "../core/User.h"
#include <stdexcept>

// Права определены как битовые флаги (степени двойки) в ПРАВИЛЬНОМ порядке
enum class Permission : unsigned int {
    NONE = 0,
    // Права, назначаемые пользователям
    READ      = 1 << 0, // 1 - Право на чтение
    WRITE     = 1 << 1, // 2 - Право на запись
    COPY_MOVE = 1 << 2, // 4 - Право на копирование и перемещение

    // Права, связанные с ролью (только для администраторов)
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
        // Администратор по-прежнему имеет полный доступ ко всему
        if (user.getRole() == Role::ADMIN) {
            return true;
        }

        // Проверяем права на управление пользователями (доступно только админу)
        if (requiredPermission == Permission::CREATE_USER || requiredPermission == Permission::DELETE_USER) {
            return false;
        }

        // Для всех остальных прав (файловые операции) используем битовую маску
        // Побитовое "И" вернет ненулевое значение, только если нужный бит установлен
        return (user.getPermissions() & static_cast<unsigned int>(requiredPermission)) != 0;
    }
};