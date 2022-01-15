#include "AutoFixHelper.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include <string>
#include <iostream>

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

std::string getDeclStr(const Decl *decl, const ASTContext &Context) {
  static PrintingPolicy print_policy(Context.getLangOpts());
  print_policy.FullyQualifiedName = 1;
  print_policy.SuppressScope = 0;
  print_policy.PrintCanonicalTypes = 0;

  std::string declString;
  llvm::raw_string_ostream stream(declString);

  decl->print(stream, print_policy);

  stream.flush();
  return declString;
}

void stripTypeString(std::string &typeStr) {
  auto pos = typeStr.find("class");
  if (pos != std::string::npos) {
    typeStr.erase(pos, 5);
  }
  pos = typeStr.find("struct");
  if (pos != std::string::npos) {
    typeStr.erase(pos, 6);
  }
}