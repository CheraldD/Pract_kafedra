#pragma once
#include "../core/ILogger.h"
#include <string>
#include <fstream>
#include <mutex> 

class FileLogger : public ILogger {
public:
   
    explicit FileLogger(const std::string& filePath);
    
    ~FileLogger();

    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;
    void log(const std::string& message) override;

private:
    std::ofstream m_logFile; 
    std::mutex m_mutex;      
};