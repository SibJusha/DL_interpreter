#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <stack>
#include <utility>
#include <memory>

class eval_error : std::exception {
public:
    [[nodiscard]] static const char* error() noexcept {
        return "Evaluation's got errors";
    }
};

class getValue_error : std::exception {
public:
    static const char* error() noexcept {
        return "getValue() was called not from class Val";
    }
};

class parse_error : std::exception {
public:
    static const char* error() noexcept {
        return "parse error";
    }
};


template <typename Returnable>
class Expression {
    /*...*/
public:

    virtual Returnable eval() = 0;

    [[nodiscard]] virtual int getValue() const = 0;

};

class Val : public Expression<Val> {
    int integer;
public:
    explicit Val(int n) : integer(n) {}

    Val() : Val(0) {}

    Val eval() override {
        return *this;
    }

    Val& operator= (int n) {
        *this = Val(n);
        return *this;
    }

    [[nodiscard]] int getValue() const override {
        return integer;
    }

    static int parse () {
        int get;
        try {
            std::cin >> get;
        } catch (...) {
            throw parse_error();
        }
        return get;
    }

};

class Var : public Expression<Val> {
    std::string id;
public:

    explicit Var(std::string id):
        id(std::move(id))
    {}

    Val eval() override;

    bool operator==(const Var& that) {
        return id == that.id;
    }

    [[nodiscard]] std::string get_id () const {
        return id;
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

    static std::string parse () {
        std::string get;
        std::cin >> get;
        return get;
    }

};


static std::unordered_map<std::string, Expression<Val>*> env;
static std::unique_ptr<Expression<Val>> fromEnv(const std::string& V) {
    Expression<Val> *found;
    try {
        found = env.at(V);
    } catch (const std::out_of_range &exception) {
        throw exception;
    }
    return std::unique_ptr<Expression<Val>>(found);
}


Val Var::eval() {
    Val found;
    try {
        found = fromEnv(id)->getValue();
    } catch (const std::exception& exception) {
        throw exception;
    }
    return found;
}

std::unique_ptr<Expression<Val>> Parser();

class Add : public Expression<Val> {
    std::unique_ptr<Expression<Val>> left;
    std::unique_ptr<Expression<Val>> right;
public:

    Add(std::unique_ptr<Expression<Val>> left,
            std::unique_ptr<Expression<Val>> right) :
        left(std::move(left)),
        right(std::move(right))
    {}

    Add() : Add(nullptr, nullptr) {}

    Val eval() override {
        Val result;
        try {
            result = Val(left->eval().getValue() +
                    right->eval().getValue());
        } catch (const std::exception& exception) {
            throw exception;
        }
        return result;
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

    void setLeft(std::unique_ptr<Expression<Val>> left_) {
        left = std::move(left_);
    }

    void setRight(std::unique_ptr<Expression<Val>> right_) {
        right = std::move(right_);
    }

};

class If : public Expression<Val> {
    std::unique_ptr<Expression<Val>> if_left_;
    std::unique_ptr<Expression<Val>> if_right_;
    std::unique_ptr<Expression<Val>> then_;
    std::unique_ptr<Expression<Val>> else_;
public:

    If(std::unique_ptr<Expression<Val>> if_left_,
       std::unique_ptr<Expression<Val>> if_right_,
       std::unique_ptr<Expression<Val>> then_,
       std::unique_ptr<Expression<Val>> else_) :
        if_left_(std::move(if_left_)),
        if_right_(std::move(if_right_)),
        then_(std::move(then_)),
        else_(std::move(else_))
    {}

    If() : If(nullptr, nullptr, nullptr, nullptr) {}

    Val eval() override {
        try {
            if (if_left_->eval().getValue() > if_right_->eval().getValue()) {
                return then_->eval();
            }
            return else_->eval();
        } catch (const std::exception& exception) {
            throw exception;
        }
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};

class Let : public Expression<Val> {
    std::unique_ptr<Expression<Val>> in;
public:

    Let (const std::string& id, std::unique_ptr<Expression<Val>> id_expr,
         std::unique_ptr<Expression<Val>> in) :
        in(std::move(in))
    {
        env.insert_or_assign(id, id_expr);
    }

    Val eval() override {
        Val result;
        try {
            result = in->eval();
        } catch (...) {
            throw eval_error();
        }
        return result;
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};

struct ThisIsFunction : public std::exception {};

class Function : public Expression<Val> {
    std::string id;
    Expression* func_expr;
public:

    Function(std::string  id, Expression* func_expr) :
        id(std::move(id)),
        func_expr(func_expr)
    {}

    Val eval() override {
        throw ThisIsFunction();
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};

class Call : public Expression<Val> {
    std::string arg_id;
    Expression* arg_expression;
public:

    Val eval() override {
        Val result;
        try {
            arg_expression->eval();
        } catch (const ThisIsFunction& yes) {

        }
        throw eval_error();
    }

};

/*
 * (add (val 3) (val 2))
 *
 * add val 3 val 2
 *
 * let A = val 20 in
 * let B = val 30 in
 * if var A add var B val 3
 * then val 10
 * else add var B val 1
 *
 * (let A = (val 20) in
 * (let B = (val 30) in
 * (if (var A) (add (var B) (val 3))
 * then (val 10)
 * else (add (var B) (val 1)))))
 */

std::string RemoveUnnecessary_and_GetName(std::string& str) {
    std::string result;
    for (auto ch : str)
    {
        if (ch == '(' || ch == ' ' || ch == ')') {
            continue;
        } else {
            result += ch;
        }
    }
    return result;
}

std::unique_ptr<Expression<Val>> Parser() {
    std::string current, input;
    while (current.empty()) {
        std::cin >> input;
        current = RemoveUnnecessary_and_GetName(input);
    }
    try {
        if (current == "val") {
            std::unique_ptr<Expression<Val>> result = std::make_unique<Val>(Val(Val::parse())); //add to ProgramStack maybe?
            return result;
        } else if (current == "var") {
            std::unique_ptr<Expression<Val>> result(new Var(Var::parse()));
            return result;
        } else if (current == "add") {
            auto temp = new Add();
            temp->setLeft(Parser());
            temp->setRight(Parser());
            std::unique_ptr<Expression<Val>> result(temp);
            return result;
        }
    } catch (const std::exception& exception) {
        throw exception;
    }
}

/* DumbHash(Expression_Name):
 * val = 111
 * var = 117
 * add = 103
 * if = 72
 * let = 119
 * function = 118
 * call = 112
 */

enum typeInHash {val = 111, var = 117, add = 103, _if = 72, let = 119,
        function = 118, call = 112};
size_t DumbHash (const std::string& str) {
    return str.length() + str.back();
}

std::unique_ptr<Expression<Val>> Read_and_Create() {

    std::stack<std::unique_ptr<Expression<Val>>> ParseStack;
    std::stack<typeInHash> TypeStack;
    std::unique_ptr<Expression<Val>> result;

    std::string current, input;
    while (current.empty() && !input.empty()) {
        std::cin >> input;
        current = RemoveUnnecessary_and_GetName(input);
    }
    while (!input.empty()){
        auto cur = static_cast<typeInHash>(DumbHash(current));
        try {
            switch (cur) {
                case val:
                    result = std::make_unique<Val>(Val::parse());
                    ParseStack.push(result);
                    TypeStack.push(val);
                    continue;
                case var:
                    continue;
                case add: {
                    auto temp = new Add();
                    std::unique_ptr<Expression<Val>> result_(temp);
                    result = std::move(result_);
                    ParseStack.push(result);
                    TypeStack.push(add);
                    continue;
                }
                case _if:
                    break;
                case let:
                    break;
                case function:
                    break;
                case call:
                    break;
            }
        } catch (const std::exception &exception) {
            throw exception;
        }
    }

    while (!ParseStack.empty()) {

    }

    return result;
}

int main() {
    try {
        std::cout << Read_and_Create()->eval().getValue();
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}
