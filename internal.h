#ifndef INTERNAL_H
#define INTERNAL_H

#include "include/Kaleidoscope.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include <map>

#define ANON_FUNCTION "__anon_expr"
#define DEBUG false
#define REPL_STR ">> "

using namespace llvm;
using namespace llvm::orc;

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
extern ExitOnError ExitOnErr;

extern std::map<std::string, int> BINOP_PRECEDENCE;

AllocaInst *create_entry_block_alloca(Function *function, StringRef var_name);
void initialize_modules_and_managers();

#endif
