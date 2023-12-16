#ifndef __EXPRESSIONS_H__
#define __EXPRESSIONS_H__

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

enum typeInHash {val = 1, var = 2, add = 3, _if = 4, let = 5,
    function = 6, call = 7, set = 8, block = 9};

struct Env {
    std::unordered_map<std::string, std::unordered_map<std::string,
            std::shared_ptr<Expression>>> envMap;

    std::unordered_map<std::string,std::shared_ptr<Expression>> currentEnv;

    std::shared_ptr<Expression> fromEnv(const std::string &V);
};

class Expression {
    const typeInHash type;
public:

    inline explicit Expression (typeInHash type) :
        type(type)
    {}

    /**
     * Evaluates the expression and returns a shared pointer
     * to a Val object representing the integer value.
     *
     * @return A shared pointer to a Val object representing the 
     *         integer value.
     *
     * @throws ErrorType A description of the error that can occur 
     *         during evaluation.
     */
    virtual std::shared_ptr<Expression> eval() = 0;

    virtual int get_value() const = 0;

    virtual ~Expression () = default;

    /**
     * Returns the ID of the object.
     *
     * @return the ID of the object
     * 
     * @throws parse_error for all Expression classes except Var 
     */
    virtual std::string get_id () const = 0;

    virtual std::string to_string() const = 0;

    typeInHash getType () {
        return type;
    }
};

class Val : public Expression {
    int integer;
public:
    explicit Val(int n);

    Val() : Val(0) {}

    ~Val() override = default;

    Val& operator= (int n);

    Val& operator= (const Val& that);

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::string to_string() const override;
};

class Var : public Expression {
    std::string id;
public:

    explicit Var(std::string id);

    ~Var() override = default;

    std::shared_ptr<Expression> eval() override;

    bool operator==(const Var& that);

    std::string get_id () const override;

    int get_value() const override;

    std::string to_string() const override;
};

class Add : public  Expression {
     std::shared_ptr<Expression> left;
     std::shared_ptr<Expression> right;
public:

    Add(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right);

    Add() : Add(nullptr, nullptr) {}

    ~Add() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::string to_string() const override;
};

class If : public  Expression {
     std::shared_ptr<Expression> if_left_;
     std::shared_ptr<Expression> if_right_;
     std::shared_ptr<Expression> then_;
     std::shared_ptr<Expression> else_;
public:

    If( std::shared_ptr<Expression> if_left_,  std::shared_ptr<Expression> if_right_,
        std::shared_ptr<Expression> then_,  std::shared_ptr<Expression> else_);

    If() : If(nullptr, nullptr, nullptr, nullptr) {}

    ~If() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::string to_string() const override;
};

class Let : public  Expression {
    std::string id;
    std::shared_ptr<Expression> id_expr;
    std::shared_ptr<Expression> in;
public:

    Let (std::string id, std::shared_ptr<Expression> id_expr,
         std::shared_ptr<Expression> in);

    Let();

    ~Let() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::string to_string() const override;
};

class Function : public  Expression {
    std::string arg_id;
    std::shared_ptr<Expression> funcBody;
public:

    Function(std::string id, std::shared_ptr<Expression> func_expr);

    ~Function() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::shared_ptr<Expression> getBody();

    std::string to_string() const override;
};

class Call : public  Expression {
     std::shared_ptr<Expression> func_expression;
     std::shared_ptr<Expression> arg_expression;
public:

    Call (std::shared_ptr<Expression> func,  std::shared_ptr<Expression> expr);

    Call () : Call(nullptr, nullptr) {}

    ~Call() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value() const override;

    std::string get_id() const override;

    std::string to_string() const override;
};

class Set : public Expression {
    std::string id;
    std::shared_ptr<Expression> e_val;
public:

    Set(std::string id, std::shared_ptr<Expression> expr);

    ~Set() override = default;

    std::string get_id () const override;

    std::shared_ptr<Expression> eval() override;

    int get_value () const override;

    std::string to_string() const override;
};

class Block : public Expression {
    std::vector<std::shared_ptr<Expression>> expr_array;
public:

    explicit Block(std::vector<std::shared_ptr<Expression>> expr_array);

    ~Block() override = default;

    std::shared_ptr<Expression> eval() override;

    int get_value () const override;

    std::string get_id () const override;

    std::string to_string() const override;
};

#endif // __EXPRESSIONS_H__