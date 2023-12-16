#include "expressions.h"
#include "errors.h"

struct Env {
    std::unordered_map<std::string, std::unordered_map<std::string,
            std::shared_ptr<Expression>>> envMap;

    std::unordered_map<std::string,std::shared_ptr<Expression>> currentEnv;

    std::shared_ptr<Expression> fromEnv(const std::string &V) {
        try {
            std::shared_ptr<Expression> found = currentEnv.at(V);
            return found;
        } 
        catch (const std::out_of_range& exception) {
            throw exception;
        }
    }
};

static Env env;

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

    int get_value() const override {
        return integer;
    }

    std::string get_id() const override {
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
        return env.fromEnv(id);
    }

    bool operator==(const Var& that) {
        return id == that.id;
    }

    std::string get_id () const override {
        return id;
    }

    int get_value() const override {
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
                std::make_shared<Val>(left->eval()->get_value() +
                    right->eval()->get_value());
        return result;
    }

    int get_value() const override {
        throw getValue_error();
    }

    std::string get_id() const override {
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
        
        if (if_left_->eval()->get_value() > if_right_->eval()->get_value()) {
            result = then_->eval();
        }
        else {
            result = else_->eval();
        }
        
        return result;
    }

    int get_value() const override {
        throw getValue_error();
    }

    std::string get_id() const override {
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
        auto Found_by_Id = env.currentEnv.find(id);
        
        if (Found_by_Id != env.currentEnv.end()) {
            tempEnv = Found_by_Id->second;
            env.currentEnv.erase(Found_by_Id);
        }
        
        env.currentEnv.insert({id, evalId});
        
        //adds env configuration into envMap when id function declared
        if (id_expr->getType() == function) {
            env.envMap.insert({id, env.currentEnv});
        }
        
        auto result = in->eval();
        env.currentEnv.erase(id);
        
        if (tempEnv != nullptr) {
            env.currentEnv.insert({id, tempEnv});
        }
        
        return result;
    }

    int get_value() const override {
        throw getValue_error();
    }

    std::string get_id() const override {
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

    int get_value() const override {
        throw getValue_error();
    }

    std::string get_id() const override {
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
    /*
        std::unordered_map<std::string, std::shared_ptr<Expression>> Env_in_call;
        std::shared_ptr<Expression> result;
        std::shared_ptr<Function> Func;
        std::string FuncId;
        if (func_expression->getType() == var) {
            std::shared_ptr<Expression> envFunc =
                    env.at(func_expression->get_id());
            if (envFunc->getType() == function) {
                FuncId = envFunc->get_id();
                Func = std::static_pointer_cast<Function>(envFunc->eval());
                Env_in_call.insert({func_expression->get_id(), Func});
            } else {
                throw eval_error();
            }
        } else if (func_expression->getType() == function) {
            FuncId = func_expression->get_id();
            Func = std::static_pointer_cast<Function>(func_expression->eval());
        } else {
            throw parse_error();
        }

        std::shared_ptr<Expression> evalArg = arg_expression->eval();
        std::swap(Env_in_call, env);
        env.insert({FuncId, evalArg});
        result = Func->getBody()->eval();
        std::swap(env, Env_in_call);
        return result;
    */

    /* with Lexical scope's support:*/

        std::shared_ptr<Expression> result;
        std::shared_ptr<Function> Func;
        std::string FuncId;
        bool copiedEnv = false;
        std::unordered_map<std::string, std::shared_ptr<Expression>> Env_in_call;

        if (func_expression->getType() == var) {
            std::shared_ptr<Expression> envFunc =
                                    env.currentEnv.at(func_expression->get_id());
                                    
            if (envFunc->getType() == function) {
                Env_in_call = env.envMap.at(func_expression->get_id());
                copiedEnv = true;
                FuncId = envFunc->get_id();
                Func = std::static_pointer_cast<Function>(envFunc->eval());
            } 
            else {
                throw eval_error();
            }
        } 
        else if (func_expression->getType() == function) {
            std::swap(env.currentEnv, Env_in_call);
            FuncId = func_expression->get_id();
            Func = std::static_pointer_cast<Function>(func_expression->eval());
        } 
        else {
            throw eval_error();
        }

        std::shared_ptr<Expression> tempEnv = nullptr;

        if (Env_in_call.find(FuncId) != Env_in_call.end()) {
            tempEnv = Env_in_call.at(FuncId);
            Env_in_call.erase(FuncId);
        }

        Env_in_call.insert({FuncId, arg_expression->eval()});
        result = Func->getBody()->eval();
        Env_in_call.erase(FuncId);

        if (tempEnv != nullptr) {
            Env_in_call.insert({FuncId, tempEnv});
        }
        
        if (copiedEnv) {
            for (const auto& [key, expr] : Env_in_call) {
                env.currentEnv.insert_or_assign(key, expr); //mb emplace?
            }
        }
        else {
            std::swap(Env_in_call, env.currentEnv);
        }

        return result;
    }

    int get_value() const override {
        throw getValue_error();
    }

    std::string get_id() const override {
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

    std::string get_id () const override {
        return id;
    }

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> temp;
        auto envFoundById = env.currentEnv.find(id);
        
        if (envFoundById != env.currentEnv.end()) {
            env.currentEnv.erase(envFoundById);
        }
        
        env.currentEnv.insert({id, e_val});
        return std::make_shared<Set>(id, e_val);
    }

    int get_value () const override {
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

    ~Block() override = default;

    std::shared_ptr<Expression> eval() override {
        std::shared_ptr<Expression> result;
        
        for (auto& expr : expr_array) {
            result = expr->eval();
        }
        
        return result;
    }

    int get_value () const override {
        throw getValue_error();
    }

    std::string get_id () const override {
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