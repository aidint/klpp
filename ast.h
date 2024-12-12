#ifndef AST_H
#define AST_H

#include "include/Kaleidoscope.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/Casting.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define ANON_FUNCTION "__anon_expr"
using namespace llvm;
using namespace llvm::orc;

Value *log_error_v(const char *Str);

class ExprAST {
public:
  enum ExprKind {
    NumberExpr,
    VariableExpr,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    IfExpr,
    ForExpr,
    WithExpr
  };

private:
  const ExprKind Kind;

public:
  ExprKind getKind() const { return Kind; }

  ExprAST(ExprKind Kind) : Kind(Kind) {}

  virtual ~ExprAST();
  virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == NumberExpr; }
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name);
  Value *codegen() override;
  const std::string &get_name() const { return Name; }
  static bool classof(const ExprAST *E) { return E->getKind() == VariableExpr; }
};

class BinaryExprAST : public ExprAST {
  std::string Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(std::string Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == BinaryExpr; }
};

class UnaryExprAST : public ExprAST {
  std::string Op;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(std::string Op, std::unique_ptr<ExprAST> Operand);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == UnaryExpr; }
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == CallExpr; }
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Prec = 0);
  int get_arg_size() const { return Args.size(); }
  const std::string &get_name() const;
  const std::string get_operator_name() const;
  bool is_unary_op() const;
  bool is_binary_op() const;
  unsigned get_binary_precedence() const;
  Function *codegen();
};

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body);
  const std::string &get_name() const;
  Function *codegen();
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Condition, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Condition, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else);

  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == IfExpr; }
};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, Condition, Step, Body;

public:
  ForExprAST(std::string VariableName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> Condition, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == ForExpr; }
};

using VariableVector =
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>>;
class WithExprAST : public ExprAST {
  VariableVector Variables;
  std::unique_ptr<ExprAST> Body;

public:
  WithExprAST(VariableVector Variables, std::unique_ptr<ExprAST> Body);
  Value *codegen() override;
  static bool classof(const ExprAST *E) { return E->getKind() == WithExpr; }
};

std::unique_ptr<ExprAST> log_error(const char *Str);
extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::map<std::string, AllocaInst *> NamedValues;
extern std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern std::unique_ptr<FunctionPassManager> TheFPM;
extern std::unique_ptr<LoopAnalysisManager> TheLAM;
extern std::unique_ptr<FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
extern std::unique_ptr<ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<StandardInstrumentations> TheSI;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
extern std::map<std::string, std::unique_ptr<ResourceTrackerSP>> FunctionRTs;
extern ExitOnError ExitOnErr;

extern std::map<std::string, int> BINOP_PRECEDENCE;

AllocaInst *create_entry_block_alloca(Function *function, StringRef var_name);
#endif
