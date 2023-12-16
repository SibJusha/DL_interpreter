#ifndef __PARSER_H__
#define __PARSER_H__

#include <iostream>
#include <string>
#include <memory>
#include "expressions.h"

class Parser {

    void get_clean_string(std::string &str, std::istream &input);

    int balance;

public:

    Parser() : balance(0) {}
    ~Parser() = default;

    /**
     * Reads and creates an expression from the given input stream.
     *
     * @param input the input stream to read from
     *
     * @return a shared pointer to the created expression
     *
     * @throws parse_error if there is an error parsing the input
     */
    std::shared_ptr<Expression> read_and_create(std::istream& input);

};

#endif // __PARSER_H__