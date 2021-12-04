#include "AutoFixHelper.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include <string>

std::string getExprStr(const Expr *expr, const ASTContext &Context) {
  static PrintingPolicy print_policy(Context.getLangOpts());
  print_policy.FullyQualifiedName = 1;
  print_policy.SuppressScope = 0;
  print_policy.PrintCanonicalTypes = 1;

  std::string expr_string;
  llvm::raw_string_ostream stream(expr_string);
  expr->printPretty(stream, nullptr, print_policy);
  stream.flush();
  return expr_string;
}

// TODO: Make this more general. Pass a type and return first
// child of that type or nullptr.
const InitListExpr *getInitListExpr(const Stmt *S) {
  if (const InitListExpr *initListExpr =
          llvm::dyn_cast<const InitListExpr>(S)) {
    return initListExpr;
  }
  for (const Stmt *child : S->children()) {
    if (auto res = getInitListExpr(child)) {
      return res;
    }
  }
  return nullptr;
}