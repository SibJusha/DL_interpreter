#include "parser.h"
#include "errors.h"

void Parser::get_clean_string(std::string &str, std::istream &input) {
    std::string temp;
    input >> temp;

    int balance = 0;
    for (const char& c : temp) {
        if (c == '(') {
            balance++;
        } 
        else if (c == ')') {
            balance--;
        } 
        else {
            str += c;
        }
    }
    
    if (str.empty() && balance > 0) {
        get_clean_string(str, input);
    }
}

std::shared_ptr<Expression> Parser::read_and_create(std::istream& input) {
    std::string current;
    get_clean_string(current, input);

    if (current == "val") {
        std::string integer;
        get_clean_string(integer, input);
        return std::make_shared<Val>(std::stoi(integer));
    } 
    
    if (current == "var") {
        std::string name;
        get_clean_string(name, input);
        return std::make_shared<Var>(name);
    } 
    
    if (current == "add") {
        return std::make_shared<Add>(read_and_create(input),
                                        read_and_create(input));
    } 
    
    if (current == "if") {
        std::shared_ptr<Expression> if_left = read_and_create(input);
        std::shared_ptr<Expression> if_right = read_and_create(input);
        std::string temp;
        input >> temp;

        if (temp != "then") {
            throw parse_error();
        }
        
        std::shared_ptr<Expression> if_then = read_and_create(input);
        input >> temp;
        
        if (temp != "else") {
            throw parse_error();
        }
        
        std::shared_ptr<Expression> if_else = read_and_create(input);
        return std::make_shared<If>(if_left, if_right, if_then, if_else);
    }
    
    if (current == "let") {
        std::string name;
        input >> name;
        std::string temp;
        input >> temp;

        if (temp != "=") {
            throw parse_error();
        }

        std::shared_ptr<Expression> id_expr = read_and_create(input);
        input >> temp;
        
        if (temp != "in") {
            throw parse_error();
        }

        std::shared_ptr<Expression> in_expr = read_and_create(input);
        return std::make_shared<Let>(name, id_expr, in_expr);
    }
    
    if (current == "function") {
        std::string id_name;
        input >> id_name;
        return std::make_shared<Function>(id_name, read_and_create(input));
    }
    
    if (current == "call") {
        std::shared_ptr<Expression> func = read_and_create(input);
        std::shared_ptr<Expression> arg = read_and_create(input);
        return std::make_shared<Call>(func, arg);
    }
    
    if (current == "set") {
        std::string name;
        get_clean_string(name, input);
        return std::make_shared<Set>(name, read_and_create(input));
    }
    
    if (current == "block") {
        int block_balance = balance;
        std::vector<std::shared_ptr<Expression>> expr_array;

        while (balance != block_balance - 1) {
            try {
                expr_array.insert(expr_array.end(), read_and_create(input));
            } 
            catch (parse_error&) {
                if (balance == 0) {
                    return std::make_shared<Block>(expr_array);
                } 
                throw parse_error();
            }
        }

        return std::make_shared<Block>(expr_array);
    }

    throw parse_error();
}