#include "Hash.h"
#include <functional> // для std::hash

size_t hashPassword(const std::string& password) {
    // "Соль" - это случайная строка, добавляемая к паролю перед хешированием.
    // В реальной системе она должна быть уникальной для каждого пользователя.
    // Здесь мы используем статическую соль для упрощения.
    const std::string salt = "a1b2-c3d4-e5f6-a-static-salt-for-a-university-project";
    
    // std::hash - простая хеш-функция, не предназначенная для криптографии.
    return std::hash<std::string>{}(password + salt);
}