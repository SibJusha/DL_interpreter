#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <string>
#include <stack>
#include <utility>
#include <memory>

class eval_error : std::exception {
public:
      static const char* error() noexcept {
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

enum typeInHash {val = 1, var = 2, add = 3,
        _if = 4, let = 5,
    function = 6, call = 7};

template <typename Returnable>
class Expression {
    const typeInHash type;
public:

    explicit Expression (typeInHash type) :
        type(type)
    {}

    virtual Returnable eval() = 0;

    virtual int getValue() const = 0;

    virtual ~Expression () = default;

    virtual std::string getId () const = 0;

    virtual void setLeft (Expression<Returnable>*) = 0;

    virtual void setRight (Expression<Returnable>*) = 0;

    virtual void setThen (Expression<Returnable>*) = 0;

    virtual void setElse (Expression<Returnable>*) = 0;

    virtual void setIn (Expression<Returnable>*) = 0;

    typeInHash getType () {
        return type;
    }

};

class Val : public Expression<Val> {
    int integer;
public:
    explicit Val(int n) :
        Expression(val),
        integer(n)
    {}

    Val() : Val(0) {}

    Val eval() override {
        return *this;
    }

    Val& operator= (int n) {
        *this = Val(n);
        return *this;
    }

    Val& operator= (const Val& that) {
        integer = that.integer;
        return *this;
    }

    int getValue() const override {
        return integer;
    }

    std::string getId() const override {
        throw parse_error();
    }

    void setLeft (Expression<Val>*) override {
        throw parse_error();
    }

    void setRight (Expression<Val>*) override {
        throw parse_error();
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>*) override {
        throw parse_error();
    }

};

class Var : public Expression<Val> {
    std::string id;
public:

    explicit Var(std::string id):
        Expression(var),
        id(std::move(id))
    {}

    Val eval() override;

    bool operator==(const Var& that) {
        return id == that.id;
    }

      std::string getId () const override {
        return id;
    }

      int getValue() const override {
        throw getValue_error();
    }

    void setLeft (Expression<Val>*) override {
        throw parse_error();
    }

    void setRight (Expression<Val>*) override {
        throw parse_error();
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>*) override {
        throw parse_error();
    }

};

std::unordered_map<std::string, Expression<Val>*> env;

Expression<Val>* fromEnv(const std::string& V) {
    try {
        Expression<Val>* found = env.at(V);
        return found;
    } catch (const std::out_of_range &exception) {
        throw exception;
    }
}

Val Var::eval() {
    Val found;
    try {
        found = fromEnv(id)->eval();
    } catch (const std::exception& exception) {
        throw exception;
    }
    return found;
}

class Add : public Expression<Val> {
    Expression<Val>* left;
    Expression<Val>* right;
public:

    Add(Expression<Val>* left,
            Expression<Val>* right) :
        Expression(add),
        left(left),
        right(right)
    {}

    Add() : Add(nullptr, nullptr) {}

    ~Add() override {
        delete left;
        delete right;
    }

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

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    void setLeft(Expression<Val>* left_) override {
        left = left_;
    }

    void setRight(Expression<Val>* right_) override {
        right = right_;
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>*) override {
        throw parse_error();
    }

};

class If : public Expression<Val> {
    Expression<Val>* if_left_;
    Expression<Val>* if_right_;
    Expression<Val>* then_;
    Expression<Val>* else_;
public:

    If(Expression<Val>* if_left_, Expression<Val>* if_right_,
       Expression<Val>* then_, Expression<Val>* else_) :
        Expression(_if),
        if_left_(if_left_),
        if_right_(if_right_),
        then_(then_),
        else_(else_)
    {}

    If() : If(nullptr, nullptr, nullptr, nullptr) {}

    ~If() override {
        delete if_left_;
        delete if_right_;
        delete then_;
        delete else_;
    }

    Val eval() override {
        try {
            if (if_left_->eval().getValue() >
                    if_right_->eval().getValue())
            {
                return then_->eval();
            }
            return else_->eval();
        } catch (const std::exception& exception) {
            throw exception;
        }
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    void setLeft (Expression<Val>* left_) override {
        if_left_ = left_;
    }

    void setRight (Expression<Val>* right_) override {
        if_right_ = right_;
    }

    void setThen (Expression<Val>* _then) override {
        then_ = _then;
    }

    void setElse (Expression<Val>* _else) override {
        else_ = _else;
    }

    void setIn (Expression<Val>*) override {
        throw parse_error();
    }

};

class Let : public Expression<Val> {
    std::string id;
    Expression<Val>* in;
public:

    Let (std::string  id, Expression<Val>* in) :
        Expression(let),
        id(std::move(id)),
        in(in)
    {}

    explicit Let(const std::string& id) :
        Let(id, nullptr)
    {}

    ~Let() override {
        delete in;
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

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return id;
    }

    void setLeft (Expression<Val>*) override {
        throw parse_error();
    }

    void setRight (Expression<Val>*) override {
        throw parse_error();
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>* in_) override {
        in = in_;
    }

};

class Function : public Expression<Val> {
    std::string arg_id;
    Expression<Val>* func_expr;
public:

    Function(std::string  id, Expression* func_expr) :
        Expression(function),
        arg_id(std::move(id)),
        func_expr(func_expr)
    {}

    explicit Function(std::string id) :
        Function(std::move(id), nullptr)
    {}

    ~Function() override {
        delete func_expr;
    }

    Val eval() override {
        Val result;
        try {
            result = func_expr->eval();
        } catch (...) {
            throw eval_error();
        }
        return result;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return arg_id;
    }

    void setLeft (Expression<Val>*) override {
        throw parse_error();
    }

    void setRight (Expression<Val>*) override {
        throw parse_error();
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>* in_) override {
        func_expr = in_;
    }

};

class Call : public Expression<Val> {
    Expression<Val>* func_expression;
    Expression<Val>* arg_expression;
public:

    Call (Expression<Val>* func, Expression<Val>* expr) :
        Expression(call),
        func_expression(func),
        arg_expression(expr)
    {}

    Call () : Call(nullptr, nullptr) {}

    ~Call() override {
        delete arg_expression;
    }

    Val eval() override {
        try {
            if (func_expression->getType() == var) {
                auto envFunc = env.at(func_expression->getId());
                if (envFunc->getType() == function) {
                    auto temp = arg_expression->eval();
                    Expression* tempEnv = nullptr;
                    auto FUNCTION_ID = envFunc->getId();
                    if (env.find(FUNCTION_ID) != env.end()) {
                        tempEnv = env.at(FUNCTION_ID);
                        env.erase(FUNCTION_ID);
                    }
                    env.insert({FUNCTION_ID, &temp});
                    auto result = envFunc->eval();
                    env.erase(FUNCTION_ID);
                    if (tempEnv != nullptr) {
                        env.insert({FUNCTION_ID, tempEnv});
                    }
                    return result;
                }
                throw eval_error();
            } else if (func_expression->getType() == function) {
                auto temp = arg_expression->eval();
                Expression* tempEnv = nullptr;
                auto FUNCTION_ID = func_expression->getId();
                if (env.find(FUNCTION_ID) != env.end()) {
                    tempEnv = env.at(FUNCTION_ID);
                    env.erase(FUNCTION_ID);
                }
                env.insert({FUNCTION_ID, &temp});
                auto result = func_expression->eval();
                env.erase(FUNCTION_ID);
                if (tempEnv != nullptr) {
                    env.insert({FUNCTION_ID, tempEnv});
                }
                return result;
            } else {
                throw eval_error();
            }
        } catch (...) {

            throw eval_error();
        }
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    void setLeft (Expression<Val>* func) override {
        func_expression = func;
    }

    void setRight (Expression<Val>* arg) override {
        arg_expression = arg;
    }

    void setThen (Expression<Val>*) override {
        throw parse_error();
    }

    void setElse (Expression<Val>*) override {
        throw parse_error();
    }

    void setIn (Expression<Val>* in_) override {
        throw parse_error();
    }

};

void getName(int& balance, int& top, std::string& input,
             std::string& current)
{

    current.clear();
    if (!input.empty() && balance == 0) {
        return;
    }
    if (top >= input.length()) {
       if(!std::getline(std::cin, input)) {
           return;
       }
       top = 0;
    }

    for (; top < input.length(); top++) {
        char ch = input[top];
        if (!current.empty() && ch == ' ') {
            break;
        }
        if (!(ch == '(' || ch == ' ' || ch == ')')) {
            current += ch;
        } else if (ch == '(') {
            balance++;
        } else if (ch == ')') {
            balance--;
        }
    }
    top++;

    if (current.empty() && !input.empty()) {
        getName(balance, top, input, current);
    }
}


Expression<Val>* Read_and_Create() {
    std::deque<Expression<Val> *> ParseStack;

    std::unordered_map<std::string, typeInHash> mapping;

    mapping["val"] = val;
    mapping["var"] = var;
    mapping["add"] = add;
    mapping["if"] = _if;
    mapping["let"] = let;
    mapping["function"] = function;
    mapping["call"] = call;


    std::string current, input;
    int top = 0;
    int balance = 0;
    getName(balance, top, input, current);
    for (; !input.empty() && balance != 0;
           getName(balance, top, input, current)) {
        try {
            auto cur = mapping.at(current);
            switch (cur) {
                case val: {
                    getName(balance, top, input, current);
                    auto result_ = std::make_unique<Val>(std::stoi(current));
                    Expression<Val> *result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case var: {
                    getName(balance, top, input, current);
                    auto result_ = std::make_unique<Var>(current);
                    Expression<Val> *result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case add: {
                    Expression<Val> *result = new Add();
                    ParseStack.push_back(result);
                    break;
                }
                case _if: {
                    Expression<Val> *result = new If();
                    ParseStack.push_back(result);
                    break;
                }
                case let: {
                    getName(balance, top, input, current);
                    Expression<Val> *result = new Let(current);
                    ParseStack.push_back(result);
                    break;
                }
                case function: {
                    getName(balance, top, input, current);
                    Expression<Val> *result = new Function(current);
                    ParseStack.push_back(result);
                    break;
                }
                case call: {
                    Expression<Val> *result = new Call();
                    ParseStack.push_back(result);
                    break;
                }
            }
        } catch (const std::out_of_range&) {
            continue;
        } catch (const std::exception &exception) {
            throw exception;
        }
    }

    for (int i = (int) ParseStack.size() - 1; i >= 0; i--) {
        int countDeleted = 0;
        switch (ParseStack[i]->getType()) {
            case add:
                ParseStack[i]->setLeft(ParseStack[i + 1]);
                ParseStack[i]->setRight(ParseStack[i + 2]);
                countDeleted = 2;
                break;
            case _if:
                ParseStack[i]->setLeft(ParseStack[i + 1]);
                ParseStack[i]->setRight(ParseStack[i + 2]);
                ParseStack[i]->setThen(ParseStack[i + 3]);
                ParseStack[i]->setElse(ParseStack[i + 4]);
                countDeleted = 4;
                break;
            case let:
                env.insert({ParseStack[i]->getId(),
                                     ParseStack[i + 1]});
                ParseStack[i]->setIn(ParseStack[i + 2]);
                countDeleted = 2;
                break;
            case function:
                ParseStack[i]->setIn(ParseStack[i + 1]);
                countDeleted = 1;
                break;
            case call:
                ParseStack[i]->setLeft(ParseStack[i + 1]);
                ParseStack[i]->setRight(ParseStack[i + 2]);
                countDeleted = 2;
                break;
        }
        if (countDeleted >= 1) {
            ParseStack.erase(ParseStack.cbegin() + i + 1,
                             ParseStack.cbegin() + i + countDeleted + 1);
        }
    }

    return ParseStack.front();
}

int main() {
    try {
        int result = Read_and_Create()->eval().getValue();
        std::cout << "(val " << result << ")" << std::endl;
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}
