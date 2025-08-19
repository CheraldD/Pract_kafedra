#include "FileLogger.h"
#include <chrono>    
#include <iomanip>  
#include <stdexcept> 
#include <ctime>     

FileLogger::FileLogger(const std::string& filePath) {
    m_logFile.open(filePath, std::ios_base::app);

    if (!m_logFile.is_open()) {
    
        throw std::runtime_error("CRITICAL: Failed to open log file: " + filePath);
    }
}

FileLogger::~FileLogger() {
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void FileLogger::log(const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_logFile.is_open()) {
        m_logFile << std::put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") 
                  << message 
                  << std::endl; 
    }
}