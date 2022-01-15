#ifndef AUTOFIX_HELPER_H
#define AUTOFIX_HELPER_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include <string>

using namespace llvm;
using namespace clang;

std::string getExprStr(const Expr *expr, const ASTContext &Context);
std::string getDeclStr(const Decl *decl, const ASTContext &Context);
void stripTypeString(std::string &typeStr);

template <typename T>
const T *getChildOfType(const Stmt *S) {
  if (const T *initListExpr = llvm::dyn_cast<const T>(S)) {
    return initListExpr;
  }
  for (const Stmt *child : S->children()) {
    if (auto res = getChildOfType<T>(child)) {
      return res;
    }
  }
  return nullptr;
}

#endif // AUTOFIX_HELPER_H