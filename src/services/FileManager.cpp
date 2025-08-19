#include "FileManager.h"
#include "PermissionManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>

FileManager::FileManager(ILogger& logger, const std::string& logFilePath, const std::string& userDbPath, const std::string& settingsFilePath) 
    : m_logger(logger), 
      m_logFilePath(logFilePath), 
      m_userDbPath(userDbPath),
      m_settingsFilePath(settingsFilePath) {}

namespace {
    void ensureDirectoryExists(const std::filesystem::path& path) {
        auto parentDir = path.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }
    }
} 

bool FileManager::isSystemFile(const std::string& path_str) const {
    if (!std::filesystem::exists(path_str)) {
        return false;
    }
    try {
        const std::filesystem::path path(path_str);
        if (std::filesystem::exists(m_logFilePath) && std::filesystem::equivalent(path, m_logFilePath)) {
            return true;
        }
        if (std::filesystem::exists(m_userDbPath) && std::filesystem::equivalent(path, m_userDbPath)) {
            return true;
        }
        if (std::filesystem::exists(m_settingsFilePath) && std::filesystem::equivalent(path, m_settingsFilePath)) {
            return true;
        }
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
    return false;
}

void FileManager::ensureNotSystemFileForUser(const User& actor, const std::string& path) const {
    if (actor.getRole() != Role::ADMIN && isSystemFile(path)) {
        m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался получить доступ к системному файлу: " + path);
        throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
    }
}

void FileManager::readFile(const User& actor, const std::string& filePath) {
    PermissionManager::ensure(actor, Permission::READ);
    ensureNotSystemFileForUser(actor, filePath); 

    if (!std::filesystem::exists(filePath)) {
        std::ofstream newFile(filePath); 
        if (!newFile.is_open()) {
             throw std::runtime_error("Файл не существует и не может быть создан: " + filePath);
        }
        newFile.close();
        std::cout << "Файл '" << filePath << "' не найден и был создан." << std::endl;
        m_logger.log("Пользователь '" + actor.getUsername() + "' запросил несуществующий файл '" + filePath + "'. Файл создан.");
        return;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для чтения: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::cout << "\n--- Содержимое файла " << filePath << " ---\n" << buffer.str() << "\n--- Конец файла ---\n";
    m_logger.log("Пользователь '" + actor.getUsername() + "' прочитал файл '" + filePath + "'.");
}

void FileManager::writeFile(const User& actor, const std::string& filePath, const std::string& content) {
    PermissionManager::ensure(actor, Permission::WRITE);
    ensureNotSystemFileForUser(actor, filePath); 

    try {
        ensureDirectoryExists(filePath);
    } catch (const std::filesystem::filesystem_error& e) {
         throw std::runtime_error("Не удалось создать директорию для файла. " + std::string(e.what()));
    }

    std::ofstream file(filePath, std::ios_base::app);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для записи: " + filePath);
    }

    file << content << std::endl;
    if (!file) {
        throw std::runtime_error("Произошла ошибка во время записи в файл: " + filePath);
    }
    
    m_logger.log("Пользователь '" + actor.getUsername() + "' дозаписал в файл '" + filePath + "'.");
}

void FileManager::copyFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    ensureNotSystemFileForUser(actor, sourceStr); 
    
    if(isSystemFile(destStr) && actor.getRole() != Role::ADMIN){
         m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался перезаписать системный файл: " + destStr);
         throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
    }

    try {
        std::filesystem::path sourcePath(sourceStr);
        std::filesystem::path destPath(destStr);
        std::filesystem::path finalDestPath = destPath;
        if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
            finalDestPath = destPath / sourcePath.filename();
        }
        ensureDirectoryExists(finalDestPath); 
        std::filesystem::copy(sourcePath, finalDestPath, std::filesystem::copy_options::overwrite_existing);
        m_logger.log("Пользователь '" + actor.getUsername() + "' скопировал файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Операция копирования не удалась. " + std::string(e.what()));
    }
}

void FileManager::moveFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);
    ensureNotSystemFileForUser(actor, sourceStr); 

     if(isSystemFile(destStr) && actor.getRole() != Role::ADMIN){
         m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался перезаписать системный файл: " + destStr);
         throw std::runtime_error("Доступ к системным файлам разрешен только администраторам.");
    }
    
    try {
        std::filesystem::path sourcePath(sourceStr);
        std::filesystem::path destPath(destStr);
        std::filesystem::path finalDestPath = destPath;
        if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
            finalDestPath = destPath / sourcePath.filename();
        }
        ensureDirectoryExists(finalDestPath);
        std::filesystem::rename(sourcePath, finalDestPath);
        m_logger.log("Пользователь '" + actor.getUsername() + "' переместил файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Операция перемещения не удалась. " + std::string(e.what()));
    }
}