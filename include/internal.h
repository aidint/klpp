#ifndef INTERNAL_H
#define INTERNAL_H

#include "Kaleidoscope.h"
#include "lex.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/IR/DIBuilder.h"
#include <map>

#define ANON_FUNCTION "__anon_expr"
#define VERBOSE false
#define REPL_STR ">> "
#define UNIT_TERMINATOR -128

using namespace llvm;
using namespace llvm::orc;

extern bool DEBUG;

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

extern TargetMachine * TheTargetMachine;
extern std::unique_ptr<DIBuilder> DBuilder;

extern std::map<std::string, int> BINOP_PRECEDENCE;

AllocaInst *create_entry_block_alloca(Function *function, StringRef var_name);
void initialize_modules_and_managers_for_jit();

inline void set_lex_source(std::unique_ptr<std::istream> source_stream) {
  reset_lex_loc();
  TheSource->set_source(std::move(source_stream));
}

void initialize_module_for_compilation();

#endif
