#pragma once
#include "../core/ILogger.h"
#include "../core/User.h"
#include <string>

class FileManager {
public:
    // Конструктор теперь принимает пути к двум защищаемым системным файлам
    explicit FileManager(ILogger& logger, const std::string& logFilePath, const std::string& userDbPath);

    void readFile(const User& actor, const std::string& filePath);
    void writeFile(const User& actor, const std::string& filePath, const std::string& content);
    void copyFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    void moveFile(const User& actor, const std::string& sourcePath, const std::string& destPath);
    
private:
    ILogger& m_logger;
    std::string m_logFilePath;
    std::string m_userDbPath; // Путь к файлу с данными пользователей
};