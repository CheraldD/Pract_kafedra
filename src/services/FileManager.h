#pragma once
#include "../core/ILogger.h"
#include "../core/User.h"
#include <string>

class FileManager {
public:
    explicit FileManager(ILogger& logger, const std::string& logFilePath);

    /**
     * @brief Читает содержимое файла и выводит его в консоль.
     * Если файл не существует, создает его.
     */
    void readFile(const User& actor, const std::string& filePath);

    void writeFile(const User& actor, const std::string& filePath, const std::string& content);

    /**
     * @brief Копирует файл. Создает директорию назначения, если она не существует.
     */
    void copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
    /**
     * @brief Перемещает файл. Создает директорию назначения, если она не существует.
     */
    void moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
private:
    ILogger& m_logger;
    std::string m_logFilePath;
};