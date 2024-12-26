#include "ast.h"
#include "lex.h"

std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::map<std::string, ResourceTrackerSP *> FunctionRTs;

ExprAST::~ExprAST() = default;
NumberExprAST::NumberExprAST(double Val) : ExprAST(NumberExpr), Val(Val) {}
VariableExprAST::VariableExprAST(const std::string &Name)
    : ExprAST(VariableExpr), Name(Name) {}

// Binary and Unary Expressions
BinaryExprAST::BinaryExprAST(SourceLocation OpLoc, std::string Op,
                             std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS)
    : ExprAST(BinaryExpr, OpLoc), Op(Op), LHS(std::move(LHS)),
      RHS(std::move(RHS)) {}

UnaryExprAST::UnaryExprAST(std::string Op, std::unique_ptr<ExprAST> Operand)
    : ExprAST(UnaryExpr), Op(Op), Operand(std::move(Operand)) {}

// PrototypeAST
PrototypeAST::PrototypeAST(SourceLocation DefLoc, const std::string &Name,
                           std::vector<std::string> Args, bool IsOperator,
                           unsigned Prec)
    : Name(Name), Args(Args), IsOperator(IsOperator), Precedence(Prec),
      LocationLine(DefLoc.line) {}

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

CallExprAST::CallExprAST(SourceLocation FnNameLoc, const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : ExprAST(CallExpr, FnNameLoc), Callee(Callee), Args(std::move(Args)) {}
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {};

const std::string &FunctionAST::get_name() const { return Proto->get_name(); }

IfExprAST::IfExprAST(std::unique_ptr<ExprAST> Condition,
                     std::unique_ptr<ExprAST> Then,
                     std::unique_ptr<ExprAST> Else)
    : ExprAST(CallExpr), Condition(std::move(Condition)), Then(std::move(Then)),
      Else(std::move(Else)) {}

ForExprAST::ForExprAST(std::string VariableName, std::unique_ptr<ExprAST> Start,
                       std::unique_ptr<ExprAST> Condition,
                       std::unique_ptr<ExprAST> Step,
                       std::unique_ptr<ExprAST> Body)
    : ExprAST(ForExpr), VarName(VariableName), Start(std::move(Start)),
      Condition(std::move(Condition)), Step(std::move(Step)),
      Body(std::move(Body)) {}

WithExprAST::WithExprAST(VariableVector Variables,
                         std::unique_ptr<ExprAST> Body)
    : ExprAST(WithExpr), Variables(std::move(Variables)),
      Body(std::move(Body)) {}
