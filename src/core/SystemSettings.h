#pragma once
#include <string>
#include <fstream>
#include <iostream>

struct SystemSettings {
 
    int maxLoginAttempts = 3;

private:
    std::string m_configFilePath; 

public:

    void load(const std::string& path) {
        m_configFilePath = path;
        std::ifstream file(m_configFilePath);
        if (file.is_open() && (file >> maxLoginAttempts)) {
            if (maxLoginAttempts <= 0) {
                maxLoginAttempts = 3; 
            }
        }
    }

    void save() const {
        if (m_configFilePath.empty()) {
            return; 
        }
        std::ofstream file(m_configFilePath, std::ios_base::trunc);
        if (file.is_open()) {
            file << maxLoginAttempts;
        } else {
            std::cerr << "Предупреждение: Не удалось сохранить файл настроек: " << m_configFilePath << std::endl;
        }
    }
};