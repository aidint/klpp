#include "lex.h"
#include <cstdlib>
#include <string>

std::string identifier_str;
double num_val;

int gettok() {
  static int last_char = ' ';

  while (isspace(last_char))
    last_char = getchar();

  // State = Identifier
  if (isalpha(last_char)) {
    identifier_str = last_char;
    while (isalnum((last_char = getchar())))
      identifier_str += last_char;

    if (identifier_str == "def")
      return tok_def;
    else if (identifier_str == "extern")
      return tok_extern;
    return tok_identifier;
  }

  if (isnumber(last_char) || last_char == '.') {
    std::string number_string;
    bool has_point = last_char == '.';
    bool has_second_point;
    number_string = last_char;

    while (isnumber((last_char = getchar())) || last_char == '.') {
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
      last_char = getchar();
    while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF)
      return gettok();
  }

  if (last_char == EOF)
    return tok_eof;

  int this_char = last_char;
  last_char = getchar();
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
