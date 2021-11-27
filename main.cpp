#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <string>
#include <stack>
#include <utility>
#include <memory>

class eval_error : std::exception {};

class getValue_error : std::exception {};

class parse_error : std::exception {};

enum typeInHash {val = 1, var = 2, add = 3,
        _if = 4, let = 5,
    function = 6, call = 7};

class Expression {
    const typeInHash type;
public:

    explicit Expression (typeInHash type) :
        type(type)
    {}

    virtual Expression* eval() = 0;

    virtual int getValue() const = 0;

    virtual ~Expression () = default;

    virtual std::string getId () const = 0;

    virtual void setLeft (Expression*) = 0;

    virtual void setRight (Expression*) = 0;

    virtual void setThen (Expression*) = 0;

    virtual void setElse (Expression*) = 0;

    virtual void setIn (Expression*) = 0;

    typeInHash getType () {
        return type;
    }

};

class Val : public Expression {
    int integer;
public:
    explicit Val(int n) :
        Expression(val),
        integer(n)
    {}

    Val() : Val(0) {}

    Expression* eval() override {
        return new Val(integer);
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

    void setLeft (Expression*) override {
        throw parse_error();
    }

    void setRight (Expression*) override {
        throw parse_error();
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression*) override {
        throw parse_error();
    }

};

class Var : public Expression {
    std::string id;
public:

    explicit Var(std::string id):
        Expression(var),
        id(std::move(id))
    {}

    Expression* eval() override;

    bool operator==(const Var& that) {
        return id == that.id;
    }

      std::string getId () const override {
        return id;
    }

      int getValue() const override {
        throw getValue_error();
    }

    void setLeft (Expression*) override {
        throw parse_error();
    }

    void setRight (Expression*) override {
        throw parse_error();
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression*) override {
        throw parse_error();
    }

};

std::unordered_map<std::string, Expression*> env;

Expression* fromEnv(const std::string& V) {
    try {
        Expression* found = env.at(V);
        return found;
    } catch (const std::out_of_range &exception) {
        throw exception;
    }
}

Expression* Var::eval() {
    return fromEnv(id)->eval();
}

class Add : public Expression {
    Expression* left;
    Expression* right;
public:

    Add(Expression* left,
            Expression* right) :
        Expression(add),
        left(left),
        right(right)
    {}

    Add() : Add(nullptr, nullptr) {}

    ~Add() override {
        delete left;
        delete right;
    }

    Expression* eval() override {
        try {
            auto Left = left->eval();
            auto Right = right->eval();
            auto result = new Val(Left->getValue() + Right->getValue());
            delete Left;
            delete Right;
            return result;
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

    void setLeft(Expression* left_) override {
        left = left_;
    }

    void setRight(Expression* right_) override {
        right = right_;
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression*) override {
        throw parse_error();
    }

};

class If : public Expression {
    Expression* if_left_;
    Expression* if_right_;
    Expression* then_;
    Expression* else_;
public:

    If(Expression* if_left_, Expression* if_right_,
       Expression* then_, Expression* else_) :
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

    Expression* eval() override {
        try {
            if (if_left_->eval()->getValue() >
                    if_right_->eval()->getValue())
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

    void setLeft (Expression* left_) override {
        if_left_ = left_;
    }

    void setRight (Expression* right_) override {
        if_right_ = right_;
    }

    void setThen (Expression* _then) override {
        then_ = _then;
    }

    void setElse (Expression* _else) override {
        else_ = _else;
    }

    void setIn (Expression*) override {
        throw parse_error();
    }

};

class Let : public Expression {
    std::string id;
    Expression* in;
public:

    Let (std::string  id, Expression* in) :
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

    Expression* eval() override {
        try {
            return in->eval();
        } catch (...) {
            throw eval_error();
        }
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return id;
    }

    void setLeft (Expression*) override {
        throw parse_error();
    }

    void setRight (Expression*) override {
        throw parse_error();
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression* in_) override {
        in = in_;
    }

};

class Function : public Expression {
    std::string arg_id;
    Expression* body;
public:

    Function(std::string  id, Expression* func_expr) :
        Expression(function),
        arg_id(std::move(id)),
        body(func_expr)
    {}

    explicit Function(std::string id) :
        Function(std::move(id), nullptr)
    {}

    ~Function() override {
        delete body;
    }

    Expression* eval() override {
        try {
            return body->eval();
        } catch (...) {
            throw eval_error();
        }
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return arg_id;
    }

    void setLeft (Expression*) override {
        throw parse_error();
    }

    void setRight (Expression*) override {
        throw parse_error();
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression* in_) override {
        body = in_;
    }

};

class Call : public Expression {
    Expression* func_expression;
    Expression* arg_expression;
public:

    Call (Expression* func, Expression* expr) :
        Expression(call),
        func_expression(func),
        arg_expression(expr)
    {}

    Call () : Call(nullptr, nullptr) {}

    ~Call() override {
        delete arg_expression;
    }

    Expression* eval() override {
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
                    env.insert({FUNCTION_ID, temp});
                    auto result = envFunc->eval();
                    env.erase(FUNCTION_ID);
                    if (tempEnv != nullptr) {
                        env.insert({FUNCTION_ID, tempEnv});
                    }
                    delete temp;
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
                env.insert({FUNCTION_ID, temp});
                auto result = func_expression->eval();
                env.erase(FUNCTION_ID);
                if (tempEnv != nullptr) {
                    env.insert({FUNCTION_ID, tempEnv});
                }
                delete temp;
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

    void setLeft (Expression* func) override {
        func_expression = func;
    }

    void setRight (Expression* arg) override {
        arg_expression = arg;
    }

    void setThen (Expression*) override {
        throw parse_error();
    }

    void setElse (Expression*) override {
        throw parse_error();
    }

    void setIn (Expression* in_) override {
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


Expression* Read_and_Create() {
    std::deque<Expression*> ParseStack;

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
                    Expression *result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case var: {
                    getName(balance, top, input, current);
                    auto result_ = std::make_unique<Var>(current);
                    Expression *result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case add: {
                    Expression *result = new Add();
                    ParseStack.push_back(result);
                    break;
                }
                case _if: {
                    Expression *result = new If();
                    ParseStack.push_back(result);
                    break;
                }
                case let: {
                    getName(balance, top, input, current);
                    Expression *result = new Let(current);
                    ParseStack.push_back(result);
                    break;
                }
                case function: {
                    getName(balance, top, input, current);
                    Expression *result = new Function(current);
                    ParseStack.push_back(result);
                    break;
                }
                case call: {
                    Expression *result = new Call();
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
        auto Expr = Read_and_Create();
        auto Eval = Expr->eval();
        std::cout << "(val " << Eval->getValue() << ")" << std::endl;
        delete Expr;
        delete Eval;
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}
