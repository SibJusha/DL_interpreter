#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <utility>
#include <memory>
#include <deque>

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

    virtual int getValue() const = 0;

    virtual ~Expression () = default;

    virtual std::string getId () const = 0;

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

    void Print() const {
        std::cout << "(val " << integer << ")";
    }

};

class Var : public Expression {
    std::string id;
public:

    explicit Var(std::string id):
        Expression(var),
        id(std::move(id))
    {}

    bool operator==(const Var& that) {
        return id == that.id;
    }

    std::string getId () const override {
        return id;
    }

    int getValue() const override {
        throw getValue_error();
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

class Add : public  Expression {
     Expression* left;
     Expression* right;
public:

    Add( Expression* left, Expression* right) :
        Expression(add),
        left(left),
        right(right)
    {}

    Add() : Add(nullptr, nullptr) {}

    ~Add() override {
        delete left;
        delete right;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    Expression* getLeft () {
        return left;
    }

    Expression* getRight () {
        return right;
    }

};

class If : public  Expression {
     Expression* if_left_;
     Expression* if_right_;
     Expression* then_;
     Expression* else_;
public:

    If( Expression* if_left_,  Expression* if_right_,
        Expression* then_,  Expression* else_) :
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

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    Expression* getLeft () {
        return if_left_;
    }

    Expression* getRight () {
        return if_right_;
    }

    Expression* getThen () {
        return then_;
    }

    Expression* getElse () {
        return else_;
    }

};

class Let : public  Expression {
    std::string id;
    Expression* in;
public:

    explicit Let (std::string id) :
        id(id),


    explicit Let (Expression* in) :
        Expression(let),
        in(in)
    {}

    explicit Let() :
        Let(nullptr)
    {}

    ~Let() override {
        delete in;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    Expression* getIn () {
        return in;
    }

    void setIn (Expression* _in) {
        in = _in;
    }

};

class Function : public  Expression {
    std::string arg_id;
    Expression* funcBody;
public:

    Function(std::string& id, Expression* func_expr) :
            Expression(function),
            arg_id(id),
            funcBody(func_expr)
    {}

    ~Function() override {
        delete funcBody;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return arg_id;
    }

    Expression* getBody () {
        return funcBody;
    }

};

class Call : public  Expression {
     Expression* func_expression;
     Expression* arg_expression;
public:

    Call ( Expression* func,  Expression* expr) :
        Expression(call),
        func_expression(func),
        arg_expression(expr)
    {}

    Call () : Call(nullptr, nullptr) {}

    ~Call() override {
        delete arg_expression;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    Expression* getFunc () {
        return func_expression;
    }

    Expression* getArg () {
        return arg_expression;
    }

};

void getName(std::string& current) {
    std::cin >> current;

    current.erase (std::remove_if (current.begin (), current.end (), [](char c)
               {
                   return c == '(' || c == ')';
               }),
               current.end ());

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
    //int balance = 0;
    getName(current);
    for (; !input.empty(); getName(current)) {
        try {
            auto cur = mapping.at(current);
            switch (cur) {
                case val: {
                    getName(current);
                    auto result_ = std::make_unique<Val>(std::stoi(current));
                    Expression*result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case var: {
                    getName(current);
                    auto result_ = std::make_unique<Var>(current);
                    Expression*result = result_.release();
                    ParseStack.push_back(result);
                    break;
                }
                case add: {
                    Expression*result = new Add();
                    ParseStack.push_back(result);
                    break;
                }
                case _if: {
                    Expression* result = new If();
                    ParseStack.push_back(result);
                    break;
                }
                case let: {
                    getName(current);
                    Let* result = new Let(current);
                    ParseStack.push_back(result);
                    break;
                }
                case function: {
                    getName(current);
                    Expression* result = new Function(current);
                    ParseStack.push_back(result);
                    break;
                }
                case call: {
                    Expression* result = new Call();
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

    /* найти сложную структуру (не val/var)
     * и считать нужные структуры выше по стеку (deque) и опустить стек*/
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

class Evaluation {

    static Val eval(Val* expr) {
        return Val(expr->getValue());
    }

    Val eval(Var* expr) {
        return Val(eval(fromEnv(expr->getId())).getValue());
    }

    Val eval(Add* expr) {
        return Val(eval(expr->getLeft()).getValue() +
                   eval(expr->getRight()).getValue());
    }

    Val eval(If* expr) {
        if (eval(expr->getLeft()).getValue() >
            eval(expr->getRight()).getValue())
        {
            return eval(expr->getThen());
        }
        return eval(expr->getElse());
    }

    Val eval(Let* expr) {
        return eval(expr->getIn());
    }

    Val eval(Function* expr) {
        return eval(expr->getBody());
    }

    Val eval(Call* expr) {
        if (expr->getFunc()->getType() == function) {
            auto temp = eval(expr->getArg());
            env.insert({expr->getFunc()->getId(),
                        (Expression*) &temp});
            auto result = eval((Function*) expr->getFunc());
            env.erase(expr->getFunc()->getId());
            return result;
        } else if (expr->getFunc()->getType() == var) {
            auto envFunc = env.at(expr->getFunc()->getId());
            if (envFunc->getType() == function) {
                auto temp = eval(expr->getArg());
                env.insert({envFunc->getId(), (Expression*) &temp});
                auto result = eval((Function*) envFunc);
                env.erase(envFunc->getId());
                return result;
            }
            throw eval_error();
        } else {
            throw eval_error();
        }
    }

public:

    Evaluation() = default;
    ~Evaluation() = default;

    Val eval(Expression *expr) {
        try {
            switch (expr->getType()) {
                case val:
                    return eval((Val*) expr);
                case var:
                    return eval((Var*) expr);
                case add:
                    return eval((Add*) expr);
                case _if:
                    return eval((If*) expr);
                case let:
                    return eval((Let*) expr);
                case function:
                    return eval((Function*) expr);
                case call:
                    return eval((Call*) expr);
            }
        } catch (...) {
            throw eval_error();
        }
    }
};

int main() {
    try {
        int top = 0; std::string input, current;
        Evaluation eval;
        eval.eval(Read_and_Create(current)).Print();
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}