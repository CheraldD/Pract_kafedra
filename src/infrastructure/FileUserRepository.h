#pragma once
#include "../core/IUserRepository.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class FileUserRepository : public IUserRepository {
public:
    explicit FileUserRepository(const std::string& dbPath);
    ~FileUserRepository() = default;

    FileUserRepository(const FileUserRepository&) = delete;
    FileUserRepository& operator=(const FileUserRepository&) = delete;

    std::shared_ptr<User> findByUsername(const std::string& username) override;
    std::vector<std::shared_ptr<User>> getAll() override;
    void add(std::shared_ptr<User> user) override;
    void update(std::shared_ptr<User> user) override;
    void remove(const std::string& username) override;

private:
    void loadFromFile();
    void saveToFile();
    
    std::string m_dbPath;
    std::map<std::string, std::shared_ptr<User>> m_usersCache;
    std::mutex m_mutex;
};