#ifndef AUTOFIX_MATCHERS_H
#define AUTOFIX_MATCHERS_H

#include "AutoFixHelper.hpp"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

class VarDeclInit : public MatchFinder::MatchCallback {
public:
  VarDeclInit(ASTContext &ASTCtx) : ASTCtx(ASTCtx) {}
  virtual void run(const MatchFinder::MatchResult &Result);

  bool warnAutoTypeBracedInit(const VarDecl *VD);
  bool warnNonAutoTypeBracedInit(const VarDecl *VD);

private:
  ASTContext &ASTCtx;
};

class TypedefDeclInit : public MatchFinder::MatchCallback {
public:
  TypedefDeclInit(ASTContext &ASTCtx) : ASTCtx(ASTCtx) {}
  virtual void run(const MatchFinder::MatchResult &Result);

private:
  ASTContext &ASTCtx;
};

class DeclInit : public MatchFinder::MatchCallback {
public:
  DeclInit(ASTContext &ASTCtx, SourceManager &SM) : ASTCtx(ASTCtx), SM(SM) {}
  virtual void run(const MatchFinder::MatchResult &Result);

private:
  ASTContext &ASTCtx;
  SourceManager &SM;
};


#endif // AUTOFIX_MATCHERS_H