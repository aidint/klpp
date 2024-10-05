#ifndef LEX_H
#define LEX_H
#include <string>

enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
};

int gettok();

extern std::string identifier_str; // Filled in if tok_identifier
extern double num_val;             // Filled in if tok_number

#endif
