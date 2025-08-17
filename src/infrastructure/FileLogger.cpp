#include "FileLogger.h"
#include <chrono>    // для получения текущего времени
#include <iomanip>   // для форматирования времени (std::put_time)
#include <stdexcept> // для генерации исключений (std::runtime_error)
#include <ctime>     // для time_t и struct tm

FileLogger::FileLogger(const std::string& filePath) {
    // Открываем файл в режиме добавления (append), чтобы не стирать старые логи.
    m_logFile.open(filePath, std::ios_base::app);
    
    // Крайне важно проверить, удалось ли открыть файл.
    // Если логгер не работает, вся система находится в непредсказуемом состоянии.
    if (!m_logFile.is_open()) {
        // Пробрасываем исключение, которое должно быть обработано на
        // самом верхнем уровне приложения (например, в main.cpp).
        throw std::runtime_error("CRITICAL: Failed to open log file: " + filePath);
    }
}

FileLogger::~FileLogger() {
    // Деструктор std::ofstream автоматически закроет файл,
    // но явный вызов close() считается хорошей практикой.
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void FileLogger::log(const std::string& message) {
    // 1. Получаем текущее системное время
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // 2. Преобразуем его в структуру tm для форматирования.
    // Используем потокобезопасные версии `localtime`, чтобы избежать гонок данных,
    // если приложение станет многопоточным.
    std::tm tm_buf;
#ifdef _WIN32
    // Windows предоставляет _s-версии функций
    localtime_s(&tm_buf, &time_t_now);
#else
    // POSIX-системы (Linux, macOS) предоставляют _r-версии
    localtime_r(&time_t_now, &tm_buf);
#endif

    // 3. Захватываем мьютекс.
    // std::lock_guard - это RAII-обертка, которая автоматически
    // освободит мьютекс при выходе из области видимости, даже если
    // будет брошено исключение. Это делает код безопасным и простым.
    std::lock_guard<std::mutex> lock(m_mutex);

    // 4. Записываем отформатированное время и сообщение в файл.
    // Формат: [ГГГГ-ММ-ДД ЧЧ:ММ:СС] <сообщение>
    if (m_logFile.is_open()) {
        m_logFile << std::put_time(&tm_buf, "[%Y-%m-%d %H:%M:%S] ") 
                  << message 
                  << std::endl; // std::endl добавляет '\n' и сбрасывает буфер файла,
                                // гарантируя немедленную запись на диск.
    }
}