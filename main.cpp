#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <functional>
#include <stdexcept>

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

    [[nodiscard]] int getValue() const override {
        return integer;
    }

};

class Var : public Expression<Val> {
    std::string id;
public:

    explicit Var(const std::string& id):
        id(id)
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

};

namespace std {
    template<> struct hash<Var> {
        std::size_t operator()(const Var &v) const {
            return hash<std::string>{}(v.get_id());
        }
    };
}

struct Env {

    static std::unordered_map<Var, Val> env;

    static Val fromEnv(const Var &V) {
        Val found;
        try {
            found = env.at(V);
        } catch (const std::out_of_range& exception) {
            throw exception;
        }
        return found;
    }

};

Val Var::eval() {
    Val found;
    try {
        found = Env::fromEnv(*this);
    } catch (const std::out_of_range& exception) {
        throw eval_error();
    }
    return found;
}

class Add : public Expression<Val> {
    Expression* left;
    Expression* right;
public:

    Add(Expression* left, Expression* right) :
        left(left),
        right(right)
    {}

    Add() : Add(nullptr, nullptr) {}

    Val eval() override {
        Val result;
        try {
            result = Val(left->eval().getValue() + right->eval().getValue());
        } catch (...) {
            throw eval_error();
        }
        return result;
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};

class If : public Expression<Val> {
    Expression* if_left_;
    Expression* if_right_;
    Expression* then_;
    Expression* else_;
public:

    If(Expression* if_left_, Expression* if_right_,
            Expression* then_, Expression* else_) :
        if_left_(if_left_),
        if_right_(if_right_),
        then_(then_),
        else_(else_)
    {}

    If() : If(nullptr, nullptr, nullptr, nullptr) {}

    Val eval() override {
        try {
            if (if_left_->eval().getValue() > if_right_->eval().getValue()) {
                return then_->eval();
            }
            return else_->eval();
        } catch (...) {
            throw eval_error();
        }
    }

    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};

class Let : public Expression<Val> {
    Expression* in;
public:

    Let (const std::string& id, int integer, Expression* in) :
        in(in)
    {
        Env::env.insert_or_assign(Var(id), Val(integer));
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

class Function : public Expression<Val> {
    std::string id;
    Expression* func_expr;
public:

    Function() :


    [[nodiscard]] int getValue() const override {
        throw getValue_error();
    }

};



int main() {

    return 0;
}
