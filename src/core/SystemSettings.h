#pragma once
#include <string>
#include <fstream>
#include <iostream>

/**
 * @brief Структура для хранения глобальных настроек системы.
 * Это позволяет централизованно управлять параметрами, которые могут
 * изменяться администратором во время выполнения программы.
 */
struct SystemSettings {
    /**
     * @brief Максимальное количество последовательных неудачных попыток входа,
     * после которого аккаунт пользователя будет заблокирован.
     * Значение по умолчанию: 3.
     */
    int maxLoginAttempts = 3;

private:
    std::string m_configFilePath; // Путь к файлу для сохранения настроек

public:
    /**
     * @brief Загружает настройки из файла. Если файл не существует или пуст,
     * используются значения по умолчанию.
     * @param path Путь к файлу настроек.
     */
    void load(const std::string& path) {
        m_configFilePath = path;
        std::ifstream file(m_configFilePath);
        if (file.is_open() && (file >> maxLoginAttempts)) {
            // Значение успешно прочитано
            if (maxLoginAttempts <= 0) {
                maxLoginAttempts = 3; // Восстанавливаем безопасное значение по умолчанию
            }
        }
        // Если файл не открылся или пуст, просто используем значение по умолчанию.
    }

    /**
     * @brief Сохраняет текущие настройки в файл.
     */
    void save() const {
        if (m_configFilePath.empty()) {
            return; // Некуда сохранять, если путь не был задан
        }
        // Открываем файл для перезаписи (trunc)
        std::ofstream file(m_configFilePath, std::ios_base::trunc);
        if (file.is_open()) {
            file << maxLoginAttempts;
        } else {
            // В случае ошибки выводим предупреждение. Логгер здесь недоступен.
            std::cerr << "Предупреждение: Не удалось сохранить файл настроек: " << m_configFilePath << std::endl;
        }
    }
};