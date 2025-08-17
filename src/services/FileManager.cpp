#include "FileManager.h"
#include "PermissionManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>

// Конструктор, который принимает логгер и путь к файлу журнала для его защиты
FileManager::FileManager(ILogger& logger, const std::string& logFilePath) 
    : m_logger(logger), m_logFilePath(logFilePath) {}

namespace {
    /**
     * @brief ВАЖНАЯ ФУНКЦИЯ: Создает родительские директории для указанного пути.
     * 
     * Проверяет, существует ли родительский каталог для файла,
     * и если нет, рекурсивно создает всю необходимую структуру.
     * @param path Путь к конечному файлу.
     */
    void ensureDirectoryExists(const std::filesystem::path& path) {
        auto parentDir = path.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            // Эта команда создает все несуществующие папки в пути
            std::filesystem::create_directories(parentDir);
        }
    }
}

void FileManager::readFile(const User& actor, const std::string& filePath) {
    PermissionManager::ensure(actor, Permission::READ);

    if (std::filesystem::exists(filePath) && std::filesystem::equivalent(filePath, m_logFilePath)) {
        if (actor.getRole() != Role::ADMIN) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался прочитать файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала разрешен только администраторам.");
        }
    }

    // Если файл не существует, он создается
    if (!std::filesystem::exists(filePath)) {
        std::ofstream newFile(filePath); 
        if (!newFile.is_open()) {
             m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' пытался прочитать несуществующий файл '" + filePath + "', но создать его не удалось.");
             throw std::runtime_error("Файл не существует и не может быть создан: " + filePath + ". Проверьте права доступа.");
        }
        newFile.close();
        std::cout << "Файл '" << filePath << "' не найден и был создан." << std::endl;
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' запросил чтение несуществующего файла '" + filePath + "'. Файл создан.");
        return;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог прочитать файл '" + filePath + "'. Причина: Отказано в доступе ОС.");
        throw std::runtime_error("Не удалось открыть файл для чтения: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    
    std::cout << "\n--- Содержимое файла " << filePath << " ---\n"
              << buffer.str()
              << "\n--- Конец файла ---\n";

    m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' прочитал файл '" + filePath + "'.");
}

void FileManager::writeFile(const User& actor, const std::string& filePath, const std::string& content) {
    PermissionManager::ensure(actor, Permission::WRITE);

    if (std::filesystem::exists(filePath) && std::filesystem::equivalent(filePath, m_logFilePath)) {
        if (actor.getRole() != Role::ADMIN) {
             m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался записать в файл журнала.");
             throw std::runtime_error("Доступ к файлу журнала разрешен только администраторам.");
        }
    }

    try {
        // Убеждаемся, что директория для файла существует (создаем, если нужно)
        ensureDirectoryExists(filePath);
    } catch (const std::filesystem::filesystem_error& e) {
         m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: не удалось создать директорию назначения. " + e.what());
         throw std::runtime_error("Операция записи не удалась. Убедитесь, что у вас есть права на запись в эту директорию. Системная ошибка: " + std::string(e.what()));
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Не удалось открыть файл (возможно, нет прав доступа).");
        throw std::runtime_error("Не удалось открыть файл для записи: " + filePath + ". Проверьте права доступа.");
    }

    file << content;
    
    if (!file) {
         m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог записать в файл '" + filePath + "'. Причина: Произошла ошибка во время записи.");
        throw std::runtime_error("Произошла ошибка во время записи в файл: " + filePath);
    }
    
    m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' записал " + std::to_string(content.length()) + " байт в файл '" + filePath + "'.");
}

void FileManager::copyFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);

    if (actor.getRole() != Role::ADMIN) {
        if ((std::filesystem::exists(sourceStr) && std::filesystem::equivalent(sourceStr, m_logFilePath)) ||
            (std::filesystem::exists(destStr) && std::filesystem::equivalent(destStr, m_logFilePath))) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался скопировать файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала запрещен.");
        }
    }

    std::filesystem::path sourcePath(sourceStr);
    std::filesystem::path destPath(destStr);
    std::filesystem::path finalDestPath = destPath;

    // Если путь назначения - существующая папка, копируем файл внутрь
    if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
        finalDestPath = destPath / sourcePath.filename();
    }

    try {
        // **КЛЮЧЕВОЙ МОМЕНТ**: Гарантируем, что родительская директория для
        // конечного файла существует. Если нет — она будет создана.
        ensureDirectoryExists(finalDestPath); 
        
        const auto options = std::filesystem::copy_options::overwrite_existing;
        std::filesystem::copy(sourcePath, finalDestPath, options);
        
        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' скопировал файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог скопировать файл из '" + sourceStr + "'. Причина: " + e.what());
        throw std::runtime_error("Операция копирования не удалась. Проверьте путь и права доступа. Системная ошибка: " + std::string(e.what()));
    }
}

void FileManager::moveFile(const User& actor, const std::string& sourceStr, const std::string& destStr) {
    PermissionManager::ensure(actor, Permission::COPY_MOVE);

    if (actor.getRole() != Role::ADMIN) {
       if ((std::filesystem::exists(sourceStr) && std::filesystem::equivalent(sourceStr, m_logFilePath)) ||
           (std::filesystem::exists(destStr) && std::filesystem::equivalent(destStr, m_logFilePath))) {
            m_logger.log("ОТКАЗ: Пользователь '" + actor.getUsername() + "' попытался переместить файл журнала.");
            throw std::runtime_error("Доступ к файлу журнала запрещен.");
        }
    }

    std::filesystem::path sourcePath(sourceStr);
    std::filesystem::path destPath(destStr);
    std::filesystem::path finalDestPath = destPath;
    
    // Если путь назначения - существующая папка, перемещаем файл внутрь
    if (std::filesystem::exists(destPath) && std::filesystem::is_directory(destPath)) {
        finalDestPath = destPath / sourcePath.filename();
    }

    try {
        // **КЛЮЧЕВОЙ МОМЕНТ**: Гарантируем, что родительская директория для
        // конечного файла существует. Если нет — она будет создана.
        ensureDirectoryExists(finalDestPath);
        
        std::filesystem::rename(sourcePath, finalDestPath);

        m_logger.log("УСПЕХ: Пользователь '" + actor.getUsername() + "' переместил файл из '" + sourceStr + "' в '" + finalDestPath.string() + "'.");
    } catch (const std::filesystem::filesystem_error& e) {
        m_logger.log("ОШИБКА: Пользователь '" + actor.getUsername() + "' не смог переместить файл из '" + sourceStr + "'. Причина: " + e.what());
        throw std::runtime_error("Операция перемещения не удалась. Проверьте путь и права доступа. Системная ошибка: " + std::string(e.what()));
    }
}