#include "AutoFixHelper.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>
#include <sstream>
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

std::string getSourceString(SourceManager &SM, SourceLocation beginLoc,
                            SourceLocation endLoc, int offset) {
  auto beginPtr = SM.getCharacterData(beginLoc);
  auto endPtr = SM.getCharacterData(endLoc);
  return std::string(beginPtr, (endPtr - beginPtr) + 1 + offset);
}

std::vector<std::string> getWordsFromString(std::string &str) {
  std::istringstream strStream(str);
  std::string word;
  std::vector<std::string> wordVec;
  while (getline(strStream, word, ' ')) {
    wordVec.push_back(word);
  }
  return wordVec;
}
