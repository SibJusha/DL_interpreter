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
    Expression* in;
public:

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

    Function(std::string id, Expression* func_expr) :
            Expression(function),
            arg_id(std::move(id)),
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

void getName(int& top, std::string& input, std::string& current) {

    current.clear();

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
        }
    }
    top++;

    if (current.empty() && !input.empty()) {
        getName(top, input, current);
    }
}

class Mapping {
    std::unordered_map<std::string, typeInHash> mapping;
public:

    Mapping() {
        mapping["val"] = val;
        mapping["var"] = var;
        mapping["add"] = add;
        mapping["if"] = _if;
        mapping["let"] = let;
        mapping["function"] = function;
        mapping["call"] = call;
    }
    ~Mapping() = default;

    typeInHash getFromMap (const std::string& str) {
        return mapping.at(str);
    }

};
Mapping map;

Expression* Read_and_Create(int& top, std::string& input, std::string& current)
{
    getName(top, input, current);
    try {
        auto cur = map.getFromMap(current);
        switch (cur) {
            case val: {
                getName(top, input, current);
                return std::make_unique<Val>(std::stoi(current)).release();
            }
            case var: {
                getName(top, input, current);
                return std::make_unique<Var>(current).release();
            }
            case add: {
                Add *result = new Add(Read_and_Create(top, input, current),
                                      Read_and_Create(top, input, current));
                return result;
            }
            case _if: {
                If *result = new If(Read_and_Create(top, input, current),
                                    Read_and_Create(top, input, current),
                                    Read_and_Create(top, input, current),
                                    Read_and_Create(top, input, current));
                return result;
            }
            case let: {
                getName(top, input, current);
                std::string added = current;
                env.insert({added,
                            Read_and_Create(top, input, current)});
                Let *result = new Let(Read_and_Create(top, input, current));
                return result;
            }
            case function: {
                getName(top, input, current);
                auto str = current;
                auto *result = std::make_unique<Function>(str,
                                                          Read_and_Create(top, input, current)).release();
                return result;
            }
            case call: {
                Call *result = new Call(Read_and_Create(top, input, current),
                                        Read_and_Create(top, input, current));
                return result;
            }
            default:
                Read_and_Create(top, input, current);
        }
    } catch (const std::out_of_range &) {}
    catch (const std::exception &exception) {
        throw exception;
    }
}

class Evaluation {

    static Expression* eval(Val* expr) {
        return new Val(expr->getValue());
    }

    Expression* eval(Var* expr) {
        return eval(fromEnv(expr->getId()));
    }

    Expression* eval(Add* expr) {
        return new Val(eval(expr->getLeft())->getValue() +
                   eval(expr->getRight())->getValue());
    }

    Expression* eval(If* expr) {
        if (eval(expr->getLeft())->getValue() >
            eval(expr->getRight())->getValue())
        {
            return eval(expr->getThen());
        }
        return eval(expr->getElse());
    }

    Expression* eval(Let* expr) {
        return eval(expr->getIn());
    }

    Expression* eval(Function* expr) {
        return eval(expr->getBody());
    }

    Expression* eval(Call* expr) {
        if (expr->getFunc()->getType() == function) {
            auto temp = eval(expr->getArg());
            Expression* tempEnv = nullptr;
            auto FuncId = expr->getFunc()->getId();
            if (env.find(FuncId) != env.end()) {
                tempEnv = env.at(FuncId);
                env.erase(FuncId);
            }
            env.insert({FuncId, temp});
            auto result = eval((Function*) expr->getFunc());
            env.erase(FuncId);
            if (tempEnv != nullptr) {
                env.insert({FuncId, tempEnv});
            }
            return result;
        } else if (expr->getFunc()->getType() == var) {
            auto envFunc = env.at(expr->getFunc()->getId());
            if (envFunc->getType() == function) {
                auto temp = eval(expr->getArg());
                Expression* tempEnv = nullptr;
                auto FuncId = envFunc->getId();
                if (env.find(FuncId) != env.end()) {
                    tempEnv = env.at(FuncId);
                    env.erase(FuncId);
                }
                env.insert({FuncId, temp});
                auto result = eval((Function*) envFunc);
                env.erase(FuncId);
                if (tempEnv != nullptr) {
                    env.insert({FuncId, tempEnv});
                }
                return result;
            }
        }
        throw eval_error();
    }

public:

    Evaluation() = default;
    ~Evaluation() = default;

    Expression* eval(Expression *expr) {
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
        Mapping map;
        int top = 0;
        std::string input, current;
        auto Expr = Read_and_Create(top, input, current);
        Evaluation Eval;
        auto temp = Eval.eval(Expr);
        std::cout << "(val " << temp->getValue() << ")" << std::endl;
        delete Expr;
        delete temp;
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}