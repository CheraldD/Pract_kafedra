#pragma once
#include "../core/ILogger.h"
#include <string>
#include <fstream>
#include <mutex> // для потокобезопасности

/**
 * @brief Конкретная реализация ILogger, которая записывает логи в файл.
 *
 * Эта реализация является потокобезопасной, что позволяет использовать
 * один экземпляр логгера из разных частей программы без риска
 * повреждения файла.
 */
class FileLogger : public ILogger {
public:
    /**
     * @brief Конструктор, который открывает файл для логгирования.
     * @param filePath Путь к файлу логов. Файл будет создан, если не существует.
     * @throws std::runtime_error если файл не может быть открыт для записи.
     */
    explicit FileLogger(const std::string& filePath);
    
    /**
     * @brief Деструктор, который корректно закрывает файл логов.
     */
    ~FileLogger();

    // Запрещаем копирование и присваивание, чтобы избежать ситуации,
    // когда два объекта пытаются управлять одним и тем же файловым дескриптором.
    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    /**
     * @brief Записывает форматированное сообщение в файл.
     *
     * Сообщение будет содержать временную метку и перенос строки.
     * @param message Текст сообщения для записи.
     */
    void log(const std::string& message) override;

private:
    std::ofstream m_logFile; // Поток для записи в файл
    std::mutex m_mutex;      // Мьютекс для синхронизации доступа к файлу
};