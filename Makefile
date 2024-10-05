CXX = clang++
CXXFLAGS = -Wall -std=c++20 -g -I/opt/homebrew/opt/llvm/include -I/opt/homebrew/opt/llvm/include/c++/v1
TARGET = parser

$(TARGET): 
	$(CXX) $(CXXFLAGS) $(TARGET).cpp lex.cpp ast.cpp codegen.cpp -o $(TARGET)

# Clean rule to remove generated files
clean:
	rm -r $(TARGET) $(TARGET).dSYM

.PHONY: all clean

