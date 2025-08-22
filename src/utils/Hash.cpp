#include "Hash.h"
#include <functional> 

size_t hashPassword(const std::string& password) {
    const std::string salt = "a1b2-c3d4-e5f6-a-static-salt-for-a-university-project";
        return std::hash<std::string>{}(password + salt);
}