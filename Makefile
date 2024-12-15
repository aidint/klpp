CXX = clang++
FILES = parser.cpp lex.cpp ast.cpp codegen.cpp external.cpp internal.cpp
CXXFLAGS = -O3 -Wall -std=c++20
DEBUGFLAGS = -g -O0 -Wall -std=c++20

ifeq ($(TARGET), kppc)
MAINFILE = compiler.cpp 
LLVM_CONF = llvm-config --cxxflags --ldflags --system-libs --libs all
COMPILATIONFLAG = -DCOMPILATION=1
else 
MAINFILE = repl.cpp 
LLVM_CONF = llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native
endif

kppc: 
	$(CXX) `$(LLVM_CONF)` $(CXXFLAGS) $(COMPILATIONFLAG) $(MAINFILE) $(FILES) -o kppc

kpp: 
	$(CXX) `$(LLVM_CONF)` $(CXXFLAGS) $(MAINFILE) $(FILES) -o kpp

lex:
	$(CXX) `$(LLVM_CONF)` $(DEBUGFLAGS) lex.cpp -o lexer

debug:
	$(CXX) `$(LLVM_CONF)` $(DEBUGFLAGS) $(COMPILATIONFLAG) $(MAINFILE) $(FILES) -o $(TARGET)

# Clean rule to remove generated files
clean:
	rm -r $(TARGET) $(TARGET).dSYM > /dev/null 2>&1

.PHONY: all clean

