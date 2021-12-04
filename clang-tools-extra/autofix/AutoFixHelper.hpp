#ifndef AUTOFIX_HELPER_H
#define AUTOFIX_HELPER_H

#include "clang/AST/ASTContext.h"
#include <string>

using namespace llvm;
using namespace clang;

std::string getExprStr(const Expr *expr, const ASTContext &Context);
const InitListExpr *getInitListExpr(const Stmt *S);

#endif // AUTOFIX_HELPER_H