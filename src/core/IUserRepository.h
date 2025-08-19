#pragma once
#include <string>
#include <memory>
#include <vector>

class User; 

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    
    virtual std::shared_ptr<User> findByUsername(const std::string& username) = 0;

    virtual std::vector<std::shared_ptr<User>> getAll() = 0;

    virtual void add(std::shared_ptr<User> user) = 0;
    
    virtual void update(std::shared_ptr<User> user) = 0;

    virtual void remove(const std::string& username) = 0;
};