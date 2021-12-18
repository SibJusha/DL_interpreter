#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <string>
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

    virtual std::shared_ptr<Expression> eval() = 0;

    virtual int getValue() const = 0;

    virtual ~Expression () = default;

    virtual std::string getId () const = 0;

    virtual std::string to_string() const = 0;

    typeInHash getType () {
        return type;
    }

};

std::unordered_map<std::string, std::shared_ptr<Expression>> env;

std::shared_ptr<Expression> fromEnv(const std::string& V) {
    try {
        std::shared_ptr<Expression> found = env.at(V);
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

    Val& operator= (int n) {
        *this = Val(n);
        return *this;
    }

    Val& operator= (const Val& that) {
        integer = that.integer;
        return *this;
    }

    std::shared_ptr<Expression> eval() override {
        return std::make_shared<Val>(integer);
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

    std::shared_ptr<Expression> eval() override {
        return fromEnv(id);
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
     std::shared_ptr<Expression> left;
     std::shared_ptr<Expression> right;
public:

    Add(std::shared_ptr<Expression> left,
        std::shared_ptr<Expression> right) :
        Expression(add),
        left(std::move(left)),
        right(std::move(right))
    {}

    Add() : Add(nullptr, nullptr) {}

    ~Add() override = default;

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> result =
                std::make_shared<Val>(left->eval()->getValue() +
                    right->eval()->getValue());
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
     std::shared_ptr<Expression> if_left_;
     std::shared_ptr<Expression> if_right_;
     std::shared_ptr<Expression> then_;
     std::shared_ptr<Expression> else_;
public:

    If( std::shared_ptr<Expression> if_left_,  std::shared_ptr<Expression> if_right_,
        std::shared_ptr<Expression> then_,  std::shared_ptr<Expression> else_) :
        Expression(_if),
        if_left_(std::move(if_left_)),
        if_right_(std::move(if_right_)),
        then_(std::move(then_)),
        else_(std::move(else_))
    {}

    If() : If(nullptr, nullptr, nullptr, nullptr) {}

    ~If() override = default;

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> result;
        if (if_left_->eval()->getValue() > if_right_->eval()->getValue()) {
            result = then_->eval();
        } else {
            result = else_->eval();
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
        return "(if " + if_left_->to_string() + " " +
                if_right_->to_string() + "\nthen " +
                then_->to_string() + "\nelse" + else_->to_string() + ")";
    }

};

class Let : public  Expression {
    std::string id;
    std::shared_ptr<Expression> id_expr;
    std::shared_ptr<Expression> in;
public:

    Let (std::string id, std::shared_ptr<Expression> id_expr,
         std::shared_ptr<Expression> in) :
            Expression(let),
            id(std::move(id)),
            id_expr(std::move(id_expr)),
            in(std::move(in))
    {}

    Let() :
        Let("", nullptr, nullptr)
    {}

    ~Let() override = default;

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> tempEnv;
        std::shared_ptr<Expression> evalId = id_expr->eval();
        auto Found_by_Id = env.find(id);
        if (Found_by_Id != env.end()) {
            tempEnv = Found_by_Id->second;
            env.erase(Found_by_Id);
        }
        env.insert({id, evalId});
        auto result = in->eval();
        env.erase(id);
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
    std::shared_ptr<Expression> funcBody;
public:

    Function(std::string id, std::shared_ptr<Expression> func_expr) :
            Expression(function),
            arg_id(std::move(id)),
            funcBody(std::move(func_expr))
    {}

    ~Function() override = default;

    std::shared_ptr<Expression> eval() override {
        return std::make_shared<Function>(arg_id, funcBody);
    }

    int getValue() const override {
        throw getValue_error();
    }

    std::string getId() const override {
        return arg_id;
    }

    std::shared_ptr<Expression> getBody () {
        return funcBody;
    }

    std::string to_string() const override {
        return "(function " + arg_id + " " +
                funcBody->to_string() + ")";
    }

};

class Call : public  Expression {
     std::shared_ptr<Expression> func_expression;
     std::shared_ptr<Expression> arg_expression;
public:

    Call (std::shared_ptr<Expression> func,  std::shared_ptr<Expression> expr) :
        Expression(call),
        func_expression(std::move(func)),
        arg_expression(std::move(expr))
    {}

    Call () : Call(nullptr, nullptr) {}

    ~Call() override = default;

    std::shared_ptr<Expression> eval() override {
        std::unordered_map<std::string, std::shared_ptr<Expression>> tempEnv;
        std::shared_ptr<Expression> result;
        std::shared_ptr<Function> Func;
        std::string FuncId;
        if (func_expression->getType() == var) {
            std::shared_ptr<Expression> envFunc = env.at(func_expression->getId());
            if (envFunc->getType() == function) {
                FuncId = envFunc->getId();
                Func = std::static_pointer_cast<Function>(envFunc->eval());
                tempEnv.insert({func_expression->getId(), Func});
            } else {
                throw eval_error();
            }
        } else if (func_expression->getType() == function) {
            FuncId = func_expression->getId();
            Func = std::static_pointer_cast<Function>(func_expression->eval());
        }

        std::shared_ptr<Expression> evalArg = arg_expression->eval();
        std::swap(tempEnv, env);
        env.insert({FuncId, evalArg});
        result = Func->getBody()->eval();
        std::swap(env, tempEnv);
        return result;

        /* version with Lexical scope (has errors though):
    *
*       std::shared_ptr<Expression> result = nullptr;
        Function* Func;
        std::string FuncId;
        if (func_expression->getType() == var) {
            std::unique_ptr<Expression>
                  envFunc(env.at(func_expression->getId())->Clone());
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

        std::shared_ptr<Expression> tempEnv = nullptr;
        if (env.find(FuncId) != env.end()) {
            tempEnv = env.at(FuncId);
            env.erase(FuncId);
        }
        env.insert({FuncId, arg_expression->eval()});
        result = Func->getBody()->eval();
        env.erase(FuncId);
        if (tempEnv != nullptr) {
            env.insert({FuncId, tempEnv});
        }
        delete Func;
        return result;*/
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

class Set : public Expression {
    std::string id;
    std::shared_ptr<Expression> e_val;
public:

    Set(std::string id, std::shared_ptr<Expression> expr) :
        Expression(set),
        id(std::move(id)),
        e_val(std::move(expr))
    {}

    ~Set() override = default;

    std::string getId () const override {
        return id;
    }

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> temp = nullptr;
        auto envFoundById = env.find(id);
        if (envFoundById != env.end()) {
            env.erase(envFoundById);
        }
        env.insert({id, e_val});
        return std::make_shared<Set>(id, e_val);
    }

    int getValue () const override {
        throw getValue_error();
    }

    std::string to_string() const override {
        return "(set " + id + " " + e_val->to_string() + ")";
    }

};

class Block : public Expression {
    std::vector<std::shared_ptr<Expression>> expr_array;
public:

    explicit Block(std::vector<std::shared_ptr<Expression>> expr_array) :
        Expression(block),
        expr_array(std::move(expr_array))
    {}

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> result;
        for (auto& expr : expr_array) {
            result = expr->eval();
        }
        return result;
    }

    int getValue () {
        throw getValue_error();
    }

    std::string getId () {
        throw eval_error();
    }

    std::string to_string() const override {
        std::string result = "(block ";
        for (const auto& expr : expr_array) {
            result += expr->to_string() + " ";
        }
        result += ")";
        return result;
    }

};

int getCleanString (std::string& str, std::istream& input) {
    int balance = 0;
    input >> str;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == '(') {
            balance++;
            str.erase(i, i+1);
            i--;
        } else if (str[i] == ')') {
            balance--;
            str.erase(i, i+1);
            i--;
        }
    }
    if (str.empty()) {
        balance += getCleanString(str, input);
    }
    return balance;
}

std::shared_ptr<Expression> Read_and_Create(std::istream& input) {
    std::string current;
    getCleanString(current, input);
    if (current == "val") {
        std::string integer;
        getCleanString(integer, input);
        return std::make_shared<Val>(std::stoi(integer));
    }
    else if (current == "var") {
        std::string name;
        getCleanString(name, input);
        return std::make_shared<Var>(name);
    }
    else if (current == "add") {
        return std::make_shared<Add>(Read_and_Create(input),
                              Read_and_Create(input));
    }
    else if (current == "if") {
        std::shared_ptr<Expression> if_left = Read_and_Create(input);
        std::shared_ptr<Expression> if_right = Read_and_Create(input);
        std::string temp;
        input >> temp;
        if (temp != "then") {
            throw parse_error();
        }
        std::shared_ptr<Expression> if_then = Read_and_Create(input);
        input >> temp;
        if (temp != "else") {
            throw parse_error();
        }
        std::shared_ptr<Expression> if_else = Read_and_Create(input);
        return std::make_shared<If>(if_left, if_right, if_then, if_else);
    }
    else if (current == "let") {
        std::string name;
        input >> name;
        std::string temp;
        input >> temp;
        if (temp != "=") {
            throw parse_error();
        }
        std::shared_ptr<Expression> id_expr = Read_and_Create(input);
        input >> temp;
        if (temp != "in") {
            throw parse_error();
        }
        std::shared_ptr<Expression> in_expr = Read_and_Create(input);
        return std::make_shared<Let>(name, id_expr, in_expr);
    }
    else if (current == "function") {
        std::string id_name;
        input >> id_name;
        return std::make_shared<Function>(id_name, Read_and_Create(input));
    }
    else if (current == "call") {
        std::shared_ptr<Expression> func = Read_and_Create(input);
        std::shared_ptr<Expression> arg = Read_and_Create(input);
        return std::make_shared<Call>(func, arg);
    }
    else if (current == "set") {
        std::string name;
        getCleanString(name, input);
        return std::make_shared<Set>(name, Read_and_Create(input));
    }
    else if (current == "block") {
        int balance = 1;
        while (balance != 0) {

        }
    }
    throw parse_error();
}

int main() {
    try {
        std::shared_ptr<Expression> Expr = Read_and_Create(std::cin);
        std::shared_ptr<Expression> Eval = Expr->eval();
        std::cout << Eval->to_string() << std::endl;
    } catch (...) {
        std::cout << "ERROR";
    }
    return 0;
}