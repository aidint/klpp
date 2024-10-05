#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include <memory>

std::unique_ptr<PrototypeAST> log_error_p(const char *Str);

#endif
