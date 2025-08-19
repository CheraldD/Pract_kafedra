#pragma once
#include <stdexcept>
class AuthenticationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class UserNotFoundException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};
class AccountLockedException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};
class InvalidCredentialsException : public AuthenticationException {
public:
    using AuthenticationException::AuthenticationException;
};