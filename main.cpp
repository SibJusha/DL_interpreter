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
    function = 6, call = 7, set = 8, block = 9};

class Expression {
    const typeInHash type;
public:

    explicit Expression (typeInHash type) :
        type(type)
    {}

    virtual Expression* Clone() = 0;

    virtual Expression* eval() = 0;

    virtual int getValue() const = 0;

    virtual ~Expression () = default;

    virtual std::string getId () const = 0;

    typeInHash getType () {
        return type;
    }

    virtual std::string to_string() const = 0;
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

class Val : public Expression {
    int integer;
public:
    explicit Val(int n) :
        Expression(val),
        integer(n)
    {}

    Val() : Val(0) {}

    ~Val() override = default;

    Expression* Clone() override {
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

    Expression* eval() override {
        return new Val(integer);
    }

    int getValue() const override {
        return integer;
    }

    std::string getId() const override {
        throw parse_error();
    }

    std::string to_string() const override {
        return "(val " + std::to_string(integer) + ")";
    }

};

class Var : public Expression {
    std::string id;
public:

    explicit Var(std::string id):
        Expression(var),
        id(std::move(id))
    {}

    ~Var() override = default;

    Expression* Clone() override {
        return new Var(id);
    }

    Expression* eval() override {
        return fromEnv(id)->eval()->Clone();
    }

    bool operator==(const Var& that) {
        return id == that.id;
    }

    std::string getId () const override {
        return id;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string to_string() const override {
        return "(var " + id + ")";
    }

};

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

    Expression* Clone() override {
        return new Add(left->Clone(), right->Clone());
    }

    Expression* eval() override {
        auto Left = left->eval();
        auto Right = right->eval();
        Expression* result = new Val(Left->getValue() + Right->getValue());
        delete Left;
        delete Right;
        return result;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    std::string to_string() const override {
        return "(add " + left->to_string() + " "
        + right->to_string() + ")";
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

    Expression* Clone() override {
        return new If(if_left_->Clone(), if_right_->Clone(),
                      then_->Clone(), else_->Clone());
    }

    Expression* eval() override {
        auto Left = if_left_->eval();
        auto Right = if_right_->eval();
        Expression* result = nullptr;
        if (Left->getValue() > Right->getValue()) {
            result = then_->eval();
        } else {
            result = else_->eval();
        }
        delete Left;
        delete Right;
        return result;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    std::string to_string() const override {
        return "(if " + if_left_->to_string() + " " +
                if_right_->to_string() + "\nthen " +
                then_->to_string() + "\nelse" + else_->to_string() + ")";
    }

};

class Let : public  Expression {
    std::string id;
    Expression* id_expr;
    Expression* in;
public:

    Let (std::string id, Expression* id_expr, Expression* in) :
        Expression(let),
        id(std::move(id)),
        id_expr(id_expr),
        in(in)
    {}

    Let() :
        Let("", nullptr, nullptr)
    {}

    ~Let() override {
        delete id_expr;
        delete in;
    }

    Expression* Clone() override {
        return new Let(id, id_expr->Clone(), in->Clone());
    }

    Expression* eval() override {
        Expression* tempEnv = nullptr;
        auto evalId = id_expr->eval();
        if (env.find(id) != env.end()) {
            tempEnv = env.at(id);
            env.erase(id);
        }
        env.insert({id, evalId});
        auto result = in->eval();
        env.erase(id);
        delete evalId;
        if (tempEnv != nullptr) {
            env.insert({id, tempEnv});
        }
        return result;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    std::string to_string() const override {
        return "(let " + id + " = " + id_expr->to_string() +
                " in " + in->to_string() + ")";
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

    Expression* Clone() override {
        return new Function(arg_id, funcBody->Clone());
    }

    Expression* eval() override {
        return Clone();
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

    std::string to_string() const override {
        return "(function " + arg_id + " " +
                funcBody->to_string() + ")";
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
        delete func_expression;
        delete arg_expression;
    }

    Expression* Clone() override {
        return new Call(func_expression->Clone(),
                        arg_expression->Clone());
    }

    Expression* eval() override {
        /*if (func_expression->getType() == function) {
            auto ArgEval = arg_expression->eval();
            Expression* tempEnv = nullptr;
            auto FuncId = func_expression->getId();
            if (env.find(FuncId) != env.end()) {
                tempEnv = env.at(FuncId);
                env.erase(FuncId);
            }
            env.insert({FuncId, ArgEval});
            auto result =
                    ((Function*)func_expression->eval())->getBody()->eval();
            env.erase(FuncId);
            if (tempEnv != nullptr) {
                env.insert({FuncId, tempEnv});
            }
            return result;
        } else if (func_expression->getType() == var) {
            auto envFunc = env.at(func_expression->getId());
            if (envFunc->getType() == function) {
                auto ArgEval = arg_expression->eval();
                Expression* tempEnv = nullptr;
                auto FuncId = envFunc->getId();
                if (env.find(FuncId) != env.end()) {
                    tempEnv = env.at(FuncId);
                    env.erase(FuncId);
                }
                env.insert({FuncId, ArgEval});
                auto result =
                        ((Function*) envFunc->eval())->getBody()->eval();
                env.erase(FuncId);
                if (tempEnv != nullptr) {
                    env.insert({FuncId, tempEnv});
                }
                return result;
            }
        }*/

        Expression* result = nullptr;
        Function* Func = nullptr;
        std::string FuncId;
        if (func_expression->getType() == var) {
            auto envFunc = env.at(func_expression->getId());
            if (envFunc->getType() == function) {
                FuncId = envFunc->getId();
                Func = (Function*) envFunc->eval();
            } else {
                throw eval_error();
            }
        } else if (func_expression->getType() == function) {
            FuncId = func_expression->getId();
            Func = (Function*) func_expression->eval();
        } else {
            throw eval_error();
        }
        Expression* tempEnv = nullptr;
        auto ArgEval = arg_expression->eval();
        if (env.find(FuncId) != env.end()) {
            tempEnv = env.at(FuncId);
            env.erase(FuncId);
        }
        env.insert({FuncId, ArgEval});
        result = Func->getBody()->eval();
        if (tempEnv != nullptr) {
            env.insert({FuncId, tempEnv});
        }
        delete Func;
        return result;
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        throw parse_error();
    }

    std::string to_string() const override {
        return "(call " + func_expression->to_string() + " " +
                arg_expression->to_string() + ")";
    }

};

/* WIP
 * class Set : public Expression {
    std::string id;
    Expression* e_val;
public:

    Set(std::string id, Expression* expr) :
        Expression(set),
        id(std::move(id)),
        e_val(expr)
    {}

    std::string getId () const override {
        return id;
    }

    Expression* Clone() override {
        return new Set(id, e_val->Clone());
    }

    Expression* eval() override {
        Expression* temp = nullptr;
        if (env.find(id) != env.end()) {
            temp = env.at(id);
            env.erase(id);
        }
        env.insert({id, e_val});
        return this;
    }

    int getValue () const override {
        throw getValue_error();
    }

    Expression* getBody () {
        return e_val;
    }

};

class Block : public Expression {
    std::vector<Expression*> expr_array;
public:

    explicit Block(std::vector<Expression*> expr_array) :
        Expression(block),
        expr_array(std::move(expr_array))
    {}

    int getValue () {
        throw getValue_error();
    }

    std::string getId () {
        throw eval_error();
    }

    std::vector<Expression*>& getArray () {
        return expr_array;
    }

};*/

void getInput(int& top, std::string& input) {
    if (top >= input.length()) {
        if(!std::getline(std::cin, input)) {
            return;
        }
        top = 0;
    }

    if (input.empty()) {
        getInput(top, input);
    }

}

std::string getName(int& top, std::string& input) {
    std::string current;
    getInput(top, input);

    for (; top < input.length(); top++) {
        char ch = input[top];
        if (!current.empty() && ch == ' ') {
            break;
        }
        if (!(ch == '(' || ch == ' ' || ch == ')' || ch == '=')) {
            current += ch;
        }
    }
    top++;

    if (current.empty() && !input.empty()) {
        current = getName(top, input);
    }
    return current;
}

Expression* Read_and_Create(int& top, std::string& input)
{
    std::string current = getName(top, input);
    try {
        if (current == "val") {
            auto name = getName(top, input);
            return std::make_unique<Val>(std::stoi(name)).release();
        }
        else if (current == "var") {
            auto name = getName(top, input);
            return std::make_unique<Var>(name).release();
        }
        else if (current == "add") {
            Add *result = new Add(Read_and_Create(top, input),
                                  Read_and_Create(top, input));
            return result;
        }
        else if (current == "if") {
            If *result = new If(Read_and_Create(top, input),
                                Read_and_Create(top, input),
                                Read_and_Create(top, input),
                                Read_and_Create(top, input));
            return result;
        }
        else if (current == "let") {
            auto name = getName(top, input);
            Let *result = new Let(name,
                                  Read_and_Create(top, input),
                                  Read_and_Create(top, input));
            return result;
        }
        else if (current == "function") {
            auto name = getName(top, input);
            auto *result = std::make_unique<Function>(name,
                                      Read_and_Create(top, input)).release();
            return result;
        }
        else if (current == "call") {
            Call *result = new Call(Read_and_Create(top, input),
                                    Read_and_Create(top, input));
            return result;
        }
        /*WIP
         * else if (current == "set") {
            auto name = getName(top, input);
            Set* result = new Set(name, Read_and_Create(top, input));
            return result;
        }
        else if (current == "block") {

        }*/
        else if (current == "else" || current == "then" || current == "=" ||
            current == "in") {
            return Read_and_Create(top, input);
        }
    } catch (const std::out_of_range &) {}
    catch (const std::exception &exception) {
        throw exception;
    }
}

int main() {
    try {
        int top = 0;
        std::string input;
        auto Expr = Read_and_Create(top, input);
        auto Eval = Expr->eval();
        std::cout << Eval->to_string() << std::endl;
        delete Expr;
        delete Eval;
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}