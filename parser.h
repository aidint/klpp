#ifndef PARSER_H
#define PARSER_H
#include "lex.h"

extern int cur_tok;
inline int get_next_token() { return cur_tok = gettok(); }
void handle_definition(), handle_extern(), handle_top_level_expression();

#endif
