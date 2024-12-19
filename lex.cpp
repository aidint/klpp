#include "lex.h"
#include <cstdio>
#include <cstdlib>
#define BUFSIZE 100

std::string identifier_str;
std::string operator_name;
double num_val;
static int buf[BUFSIZE];
static int bufp;
std::unique_ptr<SourceReader> TheSource = std::make_unique<SourceReader>();
SourceLocation cur_loc;
static SourceLocation lex_loc = {1, 0};

void reset_lex_loc() { lex_loc = {1, 0}; }

static bool is_viable_operator_char(char c) {
  if (c == '!' || c == '$' || c == '%' || c == '&' || c == ':' || c == '*' ||
      c == '/' || c == '+' || c == '-' || c == '<' || c == '>' || c == '=' ||
      c == '?' || c == '@' || c == '[' || c == ']' || c == '\\' || c == '^' ||
      c == '|' || c == '{' || c == '}' || c == '~')
    return true;
  return false;
}

[[maybe_unused]] static char putback_char(int c) {
  if (bufp == BUFSIZE)
    printf("Warning: buffer overflow");
  else
    buf[bufp++] = c;
  return c;
}

static int get_char() {
  int c;
  if (bufp)
    c = buf[--bufp];
  else
    c = TheSource->get_next_char();

  if (c == '\n' || c == '\r') {
    lex_loc.line++;
    lex_loc.col = 0;
  } else
    lex_loc.col++;

  return c;
}

// {binary | unary}<operator_name>{ }*(.*)
// returns last_char
char get_operator(char last_char) {
  operator_name = "";
  if (last_char == '`') {

    operator_name += '`';
    last_char = get_char();
    while ((isalnum(last_char) || is_viable_operator_char(last_char)) &&
           last_char != '`') {
      operator_name += last_char;
      last_char = get_char();
    }

    operator_name += last_char;
    if (last_char != '`' || operator_name == "``") {
      std::for_each(operator_name.rbegin(), operator_name.rend(), putback_char);
      operator_name = "";
    }

    return get_char(); // returns `
  }

  while (is_viable_operator_char(last_char)) {
    operator_name += last_char;
    last_char = get_char();
  }

  return last_char;
}

int gettok() {
  static int last_char = ' ';

  while (isspace(last_char))
    last_char = get_char();

  cur_loc = lex_loc;

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
      last_char = get_operator(last_char);
      return operator_name.empty() ? tok_identifier : tok_binary;
    } else if (identifier_str == "unary") {
      last_char = get_operator(last_char);
      return operator_name.empty() ? tok_identifier : tok_unary;
    } else if (identifier_str == "with")
      return tok_with;
    return tok_identifier;
  }

  if (isnumber(last_char) || last_char == '.') {
    std::string number_string;
    bool has_point = last_char == '.';
    bool has_second_point = false;
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

    num_val = std::atof(number_string.c_str());
    return tok_number;
  }

  if (last_char == '#') {
    do
      last_char = get_char();
    while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF)
      return gettok();
  }

  if (last_char == '`' || is_viable_operator_char(last_char)) {
    last_char = get_operator(last_char);
    if (operator_name != "")
      return tok_operator;
  }

  if (last_char == EOF) {
    last_char = ' ';
    return tok_eof;
  }

  int this_char = last_char;
  last_char = get_char();
  return this_char;
}

// int main() {
//   int tok;
//   while ((tok = gettok()) != tok_eof) {
//     switch (tok) {
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
//     case tok_unary:
//       printf("tok_unary: %s\n", operator_name.c_str());
//       break;
//     case tok_binary:
//       printf("tok_binary: %s\n", operator_name.c_str());
//       break;
//     case tok_operator:
//       printf("tok_operator: %s\n", operator_name.c_str());
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
