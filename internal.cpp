#include "internal.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<IRBuilder<>> Builder;
std::unique_ptr<Module> TheModule;
std::map<std::string, AllocaInst *> NamedValues;
std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;
ExitOnError ExitOnErr;

TargetMachine *TheTargetMachine;
// Binary Expression Operations
//
std::map<std::string, int> BINOP_PRECEDENCE = {
    {"=", 2}, {"<", 10}, {">", 10}, {"+", 20}, {"-", 20}, {"*", 40},
};

AllocaInst *create_entry_block_alloca(Function *function, StringRef var_name) {
  IRBuilder<> temp_builder(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
  return temp_builder.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr,
                                   var_name);
}

void initialize_modules_and_managers_for_jit() {
  // Open a new context and module.

  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my first jit", *TheContext);

  TheModule->setDataLayout(TheJIT->getDataLayout());

  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);

  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add passes
  TheFPM->addPass(InstCombinePass());
  TheFPM->addPass(ReassociatePass());
  TheFPM->addPass(GVNPass());
  TheFPM->addPass(SimplifyCFGPass());
  TheFPM->addPass(PromotePass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(
      *TheLAM, *TheFAM, *TheCGAM,
      *TheMAM); // I don't know why the other two were registerd separately

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

void initialize_module_for_compilation() {
  auto target_triple = sys::getDefaultTargetTriple();

  std::string error;
  auto target = TargetRegistry::lookupTarget(target_triple, error);

  if (!target) {
    errs() << error;
    exit(1);
  }

  auto CPU = "generic";
  auto features = "";

  TargetOptions opt;
  TheTargetMachine = target->createTargetMachine(target_triple, CPU,
                                                    features, opt, Reloc::PIC_);
  // Open a new context and module.

  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my first compiler", *TheContext);

  TheModule->setDataLayout(TheTargetMachine->createDataLayout());
  TheModule->setTargetTriple(target_triple);


  // TheModule->setDataLayout(TheJIT->getDataLayout());

  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);

  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add passes
  TheFPM->addPass(InstCombinePass());
  TheFPM->addPass(ReassociatePass());
  TheFPM->addPass(GVNPass());
  TheFPM->addPass(SimplifyCFGPass());
  TheFPM->addPass(PromotePass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(
      *TheLAM, *TheFAM, *TheCGAM,
      *TheMAM); // I don't know why the other two were registerd separately

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}
