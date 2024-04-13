#include "parser.h"
#include <memory>

int main() {
    try {
        Parser parser;
        std::shared_ptr<Expression> Expr = parser.read_and_create(std::cin);
        std::shared_ptr<Expression> Eval = Expr->eval();
        std::cout << Eval->to_string() << std::endl;
    } catch (std::exception& Exception) {
        std::cout << "ERROR" << std::endl;
        std::cout << Exception.what() << std::endl;
    }
    return 0;
}