#include "ast.h"

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<IRBuilder<>> Builder;
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;

ExprAST::~ExprAST() = default;
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}
BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args)
    : Name(Name), Args(Args) {}
CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)){};

std::unique_ptr<ExprAST> log_error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

std::string PrototypeAST::get_name() { return Name; }
