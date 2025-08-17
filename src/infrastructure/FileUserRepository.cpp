#include "FileUserRepository.h"
#include "../core/User.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <vector>

FileUserRepository::FileUserRepository(const std::string& dbPath) : m_dbPath(dbPath) {
    loadFromFile();
}

void FileUserRepository::loadFromFile() {
    std::ifstream file(m_dbPath);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string username, hash_str, role_str, locked_str, attempts_str;
        if (std::getline(ss, username, ';') &&
            std::getline(ss, hash_str, ';') &&
            std::getline(ss, role_str, ';') &&
            std::getline(ss, locked_str, ';') &&
            std::getline(ss, attempts_str, ';')) {
            try {
                auto user = std::make_shared<User>(
                    username, 
                    std::stoull(hash_str), 
                    (std::stoi(role_str) == 1) ? Role::ADMIN : Role::USER,
                    (std::stoi(locked_str) == 1), 
                    std::stoi(attempts_str)
                );
                m_usersCache[username] = user;
            } catch (const std::exception& e) {
                std::cerr << "Предупреждение: Пропуск поврежденной строки в базе данных пользователей: " << line << std::endl;
            }
        }
    }
}

void FileUserRepository::saveToFile() {
    std::ofstream file(m_dbPath, std::ios_base::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Критическая ошибка: Не удалось сохранить в файл базы данных: " + m_dbPath);
    }
    for (const auto& pair : m_usersCache) {
        const auto& user = pair.second;
        file << user->getUsername() << ";"
             << user->getPasswordHash() << ";"
             << static_cast<int>(user->getRole()) << ";"
             << user->isLocked() << ";"
             << user->getFailedLoginAttempts() << std::endl;
    }
}

std::shared_ptr<User> FileUserRepository::findByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_usersCache.find(username);
    return (it != m_usersCache.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<User>> FileUserRepository::getAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<User>> allUsers;
    allUsers.reserve(m_usersCache.size());
    for (const auto& pair : m_usersCache) {
        allUsers.push_back(pair.second);
    }
    return allUsers;
}

void FileUserRepository::add(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.count(user->getUsername())) {
        throw std::runtime_error("Пользователь с таким именем уже существует: " + user->getUsername());
    }
    m_usersCache[user->getUsername()] = user;
    saveToFile();
}

void FileUserRepository::update(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.find(user->getUsername()) == m_usersCache.end()) {
        throw std::runtime_error("Невозможно обновить несуществующего пользователя: " + user->getUsername());
    }
    m_usersCache[user->getUsername()] = user;
    saveToFile();
}

void FileUserRepository::remove(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_usersCache.erase(username) == 0) {
        throw std::runtime_error("Невозможно удалить несуществующего пользователя: " + username);
    }
    saveToFile();
}