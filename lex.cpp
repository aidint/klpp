#include <cstdlib>
#include <iostream>

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
};

static std::string identifier_str; // Filled in if tok_identifier
static double num_val;             // Filled in if tok_number
//

static int gettok() {
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

