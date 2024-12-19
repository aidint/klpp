CXX = clang++
FILES = parser.cpp lex.cpp ast.cpp codegen.cpp lib/external.cpp internal.cpp debugger.cpp
CXXFLAGS = -O3 -Wall -std=c++20
DEBUGFLAGS = -g -O0 -Wall -std=c++20
LLVM_CONF_KPPC = llvm-config --cxxflags --ldflags --system-libs --libs all
LLVM_CONF_KPP = llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native

ifeq ($(TARGET), kppc)
MAINFILE = compiler.cpp
LLVM_CONF = $(LLVM_CONF_KPPC)
COMPILATIONFLAG = -DCOMPILATION=1
else ifeq ($(TARGET), kpp)
MAINFILE = repl.cpp
LLVM_CONF = $(LLVM_CONF_KPP)
endif

all: kppc kpp
	./kppc < lib/core.kl
	mv output.s lib/core.s
	./kppc < lib/builtin.kl
	mv output.s lib/builtin.s
	cd lib; clang++ -c core.s builtin.s external.cpp
	rm -r lib/*.s
	ar rcs lib/klpp.a lib/*.o
	rm -r lib/*.o
	chmod +x kl++

kppc: 
	$(CXX) `$(LLVM_CONF_KPPC)` $(CXXFLAGS) -DCOMPILATION=1 compiler.cpp $(FILES) -o kppc

kpp: 
	$(CXX) `$(LLVM_CONF_KPP)` $(CXXFLAGS) repl.cpp $(FILES) -o kpp

lex:
	$(CXX) `$(LLVM_CONF)` $(DEBUGFLAGS) lex.cpp -o lexer

debug:
	$(CXX) `$(LLVM_CONF)` $(DEBUGFLAGS) $(COMPILATIONFLAG) $(MAINFILE) $(FILES) -o $(TARGET)

# Clean rule to remove generated files
clean:
	rm -r $(TARGET) $(TARGET).dSYM > /dev/null 2>&1

.PHONY: all clean debug

