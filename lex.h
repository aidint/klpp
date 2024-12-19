#ifndef LEX_H
#define LEX_H
#include <istream>
#include <memory>
#include <string>

struct SourceLocation {
  int line;
  int col;
};

class SourceReader {
  std::unique_ptr<std::istream> source;

public:
  void set_source(std::unique_ptr<std::istream> source_stream) {
    source = std::move(source_stream);
  }

  char get_next_char() {
    char c;
    source->get(c);
    if (c == '\0')
      return EOF;

    return c;
  }
};

enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,

  // control
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,

  // for
  // for var = start, condition, step do top_level_expression end;
  tok_for = -9,
  tok_do = -10,
  tok_end = -11,

  // user-defined operators
  tok_binary = -12,
  tok_unary = -13,
  tok_operator = -14,

  // var
  tok_with = -15
};

void reset_lex_loc();
int gettok();

extern std::string identifier_str; // Filled in if tok_identifier
extern std::string operator_name;  // Filled in if tok_unary or tok_binary
extern double num_val;             // Filled in if tok_number
extern std::unique_ptr<SourceReader> TheSource;
extern SourceLocation cur_loc;

#endif
