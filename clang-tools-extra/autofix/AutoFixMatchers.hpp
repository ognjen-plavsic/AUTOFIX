#ifndef AUTOFIX_MATCHERS_H
#define AUTOFIX_MATCHERS_H

#include "AutoFixHelper.hpp"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

class DeclInit : public MatchFinder::MatchCallback {
public:
  DeclInit(ASTContext &ASTCtx, SourceManager &SM) : ASTCtx(ASTCtx), SM(SM) {}
  virtual void run(const MatchFinder::MatchResult &Result);
  bool warnAutoTypeBracedInit(const VarDecl *VD);
  bool warnNonAutoTypeBracedInit(const VarDecl *VD);
  void warnWrongPlacedSpecifiers(const Decl *D);
  void emitWarningWithHint(std::string &msg, std::string &replacementStr,
                           SourceRange SR, SourceLocation SL);
  void emitWarningWithHintInsertion(std::string &msg, std::string &str,
                                    SourceLocation insertLoc,
                                    SourceLocation diagLoc);
  void checkWrongPlacedSpecifiers(std::string &typeStr, std::string &declString,
                                  const Decl *D);

private:
  ASTContext &ASTCtx;
  SourceManager &SM;
};

#endif // AUTOFIX_MATCHERS_H