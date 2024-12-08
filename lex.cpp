#include "lex.h"
#include <cstdlib>
#include <string>
#define BUFSIZE 100

std::string identifier_str;
std::string operator_name;
double num_val;
static int buf[BUFSIZE];
static int bufp;

static int get_char() {
  if (bufp)
    return buf[--bufp];
  return getchar();
}

[[maybe_unused]] static void putback_char(int c) {
  if (bufp == BUFSIZE)
    printf("Warning: buffer overflow");
  else
    buf[bufp++] = c;
}

int gettok() {
  static int last_char = ' ';

  while (isspace(last_char))
    last_char = get_char();

  // State = Identifier
  if (isalpha(last_char)) {
    identifier_str = last_char;
    while (isalnum((last_char = get_char())))
      identifier_str += last_char;

    if (identifier_str == "def")
      return tok_def;
    else if (identifier_str == "extern")
      return tok_extern;
    else if (identifier_str == "if")
      return tok_if;
    else if (identifier_str == "then")
      return tok_then;
    else if (identifier_str == "else")
      return tok_else;
    else if (identifier_str == "for")
      return tok_for;
    else if (identifier_str == "do")
      return tok_do;
    else if (identifier_str == "end")
      return tok_end;
    else if (identifier_str == "binary") {
      operator_name = "";
      while(last_char != ' ' && last_char != '(') {
        operator_name += last_char;
        last_char = get_char();
      }
      return operator_name.empty() ? tok_identifier : tok_binary;
    } else if (identifier_str == "unary") {
      operator_name = "";
      while(last_char != ' ' && last_char != '(') {
        operator_name += last_char;
        last_char = get_char();
      }
      return operator_name.empty() ? tok_identifier : tok_binary;
    }
    return tok_identifier;
  }

  if (isnumber(last_char) || last_char == '.') {
    std::string number_string;
    bool has_point = last_char == '.';
    bool has_second_point;
    number_string = last_char;

    while (isnumber((last_char = get_char())) || last_char == '.') {
      if (!has_second_point)
        has_second_point = has_point && last_char == '.';

      if (!has_point)
        has_point = last_char == '.';

      if (!has_second_point) {
        number_string += last_char;
      }
    }

    num_val = std::atoi(number_string.c_str());
    return tok_number;
  }

  if (last_char == '#') {
    do
      last_char = get_char();
    while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF)
      return gettok();
  }

  if (last_char == EOF)
    return tok_eof;

  int this_char = last_char;
  last_char = get_char();
  return this_char;
}

// int main() {
//   int tok;
//   while ((tok = gettok()) != tok_eof) {
//   switch (tok) {
//     case tok_def:
//       printf("tok_def\n");
//       break;
//     case tok_extern:
//       printf("tok_extern\n");
//       break;
//     case tok_identifier:
//       printf("tok_identifier: %s\n", identifier_str.c_str());
//       break;
//     case tok_number:
//       printf("tok_number: %f\n", num_val);
//       break;
//     case tok_eof:
//       printf("tok_eof\n");
//       break;
//     default:
//       printf("other: %c\n", tok);
//       break;
//     }
//   }
//   return 0;
// }
