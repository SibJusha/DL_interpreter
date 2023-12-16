#ifndef __ERRORS_H__
#define __ERRORS_H__

#include <exception>
#include <string>
#include <stdexcept>

class eval_error : std::exception {
    std::string what_str;
public:
    eval_error() : what_str("Evaluation error") {}

    const char* what() const noexcept override {
        return what_str.c_str();
    }
};

class getValue_error : std::exception {
    std::string what_str;
public:
    getValue_error() : what_str("get_value() error - not 'Val' type") {}

    const char* what() const noexcept override {
        return what_str.c_str();
    }
};

class parse_error : std::exception {
    std::string what_str;
public:
    parse_error() : what_str("Parsing error") {}

    const char* what() const noexcept override {
        return what_str.c_str();
    }
};

#endif // __ERRORS_H__