#include "ast.h"

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<IRBuilder<>> Builder;
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;
std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::map<std::string, std::unique_ptr<ResourceTrackerSP>> FunctionRTs;
ExitOnError ExitOnErr;

ExprAST::~ExprAST() = default;
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

// Binary and Unary Expressions
BinaryExprAST::BinaryExprAST(std::string Op, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

UnaryExprAST::UnaryExprAST(std::string Op, std::unique_ptr<ExprAST> Operand) : Op(Op), Operand(std::move(Operand)) {}

// PrototypeAST
PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args, bool IsOperator,
                           unsigned Prec)
    : Name(Name), Args(Args), IsOperator(IsOperator), Precedence(Prec) {}

const std::string &PrototypeAST::get_name() const { return Name; }
bool PrototypeAST::is_unary_op() const {
  return IsOperator && Args.size() == 1;
}
bool PrototypeAST::is_binary_op() const {
  return IsOperator && Args.size() == 2;
}
unsigned PrototypeAST::get_binary_precedence() const { return Precedence; }

const std::string PrototypeAST::get_operator_name() const {
  assert(is_unary_op() || is_binary_op());
  std::string search_string = is_unary_op() ? "unary" : "binary";
  return Name.substr(Name.find(search_string) + search_string.size());
}

// CallExprAST

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {};

const std::string &FunctionAST::get_name() const { return Proto->get_name(); }

IfExprAST::IfExprAST(std::unique_ptr<ExprAST> Condition,
                     std::unique_ptr<ExprAST> Then,
                     std::unique_ptr<ExprAST> Else)
    : Condition(std::move(Condition)), Then(std::move(Then)),
      Else(std::move(Else)) {}

ForExpr::ForExpr(std::string VariableName, std::unique_ptr<ExprAST> Start,
                 std::unique_ptr<ExprAST> Condition,
                 std::unique_ptr<ExprAST> Step, std::unique_ptr<ExprAST> Body)
    : VarName(VariableName), Start(std::move(Start)),
      Condition(std::move(Condition)), Step(std::move(Step)),
      Body(std::move(Body)) {}

std::unique_ptr<ExprAST> log_error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

// Binary Expression Operations
//
std::map<std::string, int> BINOP_PRECEDENCE = {
    {"<", 10}, {">", 10}, {"+", 20}, {"-", 20}, {"*", 40},
};
