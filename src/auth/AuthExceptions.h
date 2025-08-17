#pragma once
#include <stdexcept>

/**
 * @brief Базовый класс для всех исключений аутентификации.
 */
class AuthenticationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Исключение, выбрасываемое, когда пользователь не найден.
 */
class UserNotFoundException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};

/**
 * @brief Исключение, выбрасываемое, когда аккаунт пользователя заблокирован.
 */
class AccountLockedException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};

/**
 * @brief Исключение, выбрасываемое, когда предоставлен неверный пароль.
 */
class InvalidCredentialsException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};