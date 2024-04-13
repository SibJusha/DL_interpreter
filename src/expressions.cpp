#include "expressions.h"
#include "errors.h"

std::shared_ptr<Expression> Env::fromEnv(const std::string &V) {
    try {
        std::shared_ptr<Expression> found = currentEnv.at(V);
        return found;
    } catch (const std::out_of_range &exception) {
        throw exception;
    }
}

static Env env;

////////////// Val /////////////////

Val::Val(int n) :
    Expression(val),
    integer(n)
{}

Val::Val() : Val(0) {}

Val& Val::operator= (int n) {
    *this = Val(n);
    return *this;
}

Val& Val::operator= (const Val& that) {
    integer = that.integer;
    return *this;
}

std::shared_ptr<Expression> Val::eval() {
    return std::make_shared<Val>(integer);
}

int Val::get_value() const {
    return integer;
}

std::string Val::get_id() const {
    throw parse_error();
}

std::string Val::to_string() const {
    return "(val " + std::to_string(integer) + ")";
}

////////////// Var /////////////////

Var::Var(std::string id):
    Expression(var),
    id(std::move(id))
{}

std::shared_ptr<Expression> Var::eval() {
    return env.fromEnv(id);
}

bool Var::operator==(const Var& that) {
    return id == that.id;
}

std::string Var::get_id () const {
    return id;
}

int Var::get_value() const {
    throw getValue_error();
}

std::string Var::to_string() const {
    return "(var " + id + ")";
}

////////////// Add /////////////////

Add::Add(std::shared_ptr<Expression> left,
    std::shared_ptr<Expression> right) :
    Expression(add),
    left(std::move(left)),
    right(std::move(right))
{}

Add::Add() : Add(nullptr, nullptr) {}

std::shared_ptr<Expression> Add::eval() {
    std::shared_ptr<Expression> result =
            std::make_shared<Val>(left->eval()->get_value() +
                right->eval()->get_value());
    return result;
}

int Add::get_value() const {
    throw getValue_error();
}

std::string Add::get_id() const {
    throw parse_error();
}

std::string Add::to_string() const {
    return "(add " + left->to_string() + " "
    + right->to_string() + ")";
}

////////////// If /////////////////

If::If( std::shared_ptr<Expression> if_left_,  std::shared_ptr<Expression> if_right_,
    std::shared_ptr<Expression> then_,  std::shared_ptr<Expression> else_) :
    Expression(_if),
    if_left_(std::move(if_left_)),
    if_right_(std::move(if_right_)),
    then_(std::move(then_)),
    else_(std::move(else_))
{}

If::If() : If(nullptr, nullptr, nullptr, nullptr) {}

std::shared_ptr<Expression> If::eval() {
    std::shared_ptr<Expression> result;
    
    if (if_left_->eval()->get_value() > if_right_->eval()->get_value()) {
        result = then_->eval();
    }
    else {
        result = else_->eval();
    }
    
    return result;
}

int If::get_value() const  {
    throw getValue_error();
}

std::string If::get_id() const  {
    throw parse_error();
}

std::string If::to_string() const  {
    return "(if " + if_left_->to_string() + " " +
            if_right_->to_string() + "\nthen " +
            then_->to_string() + "\nelse" + else_->to_string() + ")";
}

////////////// Let /////////////////

Let::Let (std::string id, std::shared_ptr<Expression> id_expr,
        std::shared_ptr<Expression> in) :
        Expression(let),
        id(std::move(id)),
        id_expr(std::move(id_expr)),
        in(std::move(in))
{}

Let::Let() :
    Let("", nullptr, nullptr)
{}

std::shared_ptr<Expression> Let::eval()  {
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

int Let::get_value() const  {
    throw getValue_error();
}

std::string Let::get_id() const  {
    throw parse_error();
}

std::string Let::to_string() const  {
    return "(let " + id + " = " + id_expr->to_string() +
            " in " + in->to_string() + ")";
}

////////////// Function /////////////////

Function::Function(std::string id, std::shared_ptr<Expression> func_expr) :
        Expression(function),
        arg_id(std::move(id)),
        funcBody(std::move(func_expr))
{}

std::shared_ptr<Expression> Function::eval()  {
    return std::make_shared<Function>(arg_id, funcBody);
}

int Function::get_value() const  {
    throw getValue_error();
}

std::string Function::get_id() const  {
    return arg_id;
}

std::shared_ptr<Expression> Function::getBody () {
    return funcBody;
}

std::string Function::to_string() const  {
    return "(function " + arg_id + " " +
            funcBody->to_string() + ")";
}

////////////// Call /////////////////

Call::Call (std::shared_ptr<Expression> func,  std::shared_ptr<Expression> expr) :
    Expression(call),
    func_expression(std::move(func)),
    arg_expression(std::move(expr))
{}

Call::Call () : Call(nullptr, nullptr) {}

std::shared_ptr<Expression> Call::eval()  {
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

int Call::get_value() const  {
    throw getValue_error();
}

std::string Call::get_id() const  {
    throw parse_error();
}

std::string Call::to_string() const  {
    return "(call " + func_expression->to_string() + " " +
            arg_expression->to_string() + ")";
}

////////////// Set /////////////////

Set::Set(std::string id, std::shared_ptr<Expression> expr) :
    Expression(set),
    id(std::move(id)),
    e_val(std::move(expr))
{}

std::string Set::get_id () const  {
    return id;
}

std::shared_ptr<Expression> Set::eval()  {
    std::shared_ptr<Expression> temp;
    auto envFoundById = env.currentEnv.find(id);
    
    if (envFoundById != env.currentEnv.end()) {
        env.currentEnv.erase(envFoundById);
    }
    
    env.currentEnv.insert({id, e_val});
    return std::make_shared<Set>(id, e_val);
}

int Set::get_value () const  {
    throw getValue_error();
}

std::string Set::to_string() const  {
    return "(set " + id + " " + e_val->to_string() + ")";
}

////////////// Block /////////////////

Block::Block(std::vector<std::shared_ptr<Expression>> expr_array) :
    Expression(block),
    expr_array(std::move(expr_array))
{}

std::shared_ptr<Expression> Block::eval()  {
    std::shared_ptr<Expression> result;
    
    for (auto& expr : expr_array) {
        result = expr->eval();
    }
    
    return result;
}

int Block::get_value () const  {
    throw getValue_error();
}

std::string Block::get_id () const  {
    throw eval_error();
}

std::string Block::to_string() const  {
    std::string result = "(block ";
    
    for (const auto& expr : expr_array) {
        result += expr->to_string() + " ";
    }
    
    result += ")";
    return result;
}