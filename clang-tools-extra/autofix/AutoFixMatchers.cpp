#include "AutoFixMatchers.hpp"
#include <iostream>

void VarDeclInit::run(const MatchFinder::MatchResult &Result) {
  if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("varDecl")) {
    if (VD->hasInit()) {
      warnAutoTypeBracedInit(VD);
      warnNonAutoTypeBracedInit(VD);
    }
  }
}

bool VarDeclInit::warnAutoTypeBracedInit(const VarDecl *VD) {
  auto &DE = ASTCtx.getDiagnostics();
  if (auto dty = llvm::dyn_cast<clang::AutoType>(VD->getType().getTypePtr())) {
    if (!dty->isDecltypeAuto()) {
      if (VD->getInitStyle() == VarDecl::ListInit ||
          VD->getInitStyle() == VarDecl::CInit) {
        auto listInitExpr = getChildOfType<InitListExpr>(VD->getInit());
        if(!listInitExpr){
          return false;
        }
        unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Warning,
            "A variable of type auto shall not be initialized using {} "
            "or ={} braced initialization");
        std::string exprStr = getExprStr(listInitExpr, ASTCtx);
        exprStr = exprStr.substr(1, exprStr.size() - 2);
        exprStr = " = " + exprStr;

        std::string replacementStr = "auto " + VD->getNameAsString() + exprStr;
        FixItHint hint =
            FixItHint::CreateReplacement(VD->getSourceRange(), replacementStr);
        DE.Report(VD->getLocation(), ID) << hint;
        return true;
      }
    }
  }
  return false;
}

bool VarDeclInit::warnNonAutoTypeBracedInit(const VarDecl *VD) {
  auto &DE = ASTCtx.getDiagnostics();
  if (VD->getInitStyle() == VarDecl::ListInit) {
    return false;
  }
  std::string typeStr = VD->getType().getAsString();
  stripTypeString(typeStr);
  auto initListExpr = getChildOfType<InitListExpr>(VD->getInit());
  auto cxxConstructExpr = getChildOfType<CXXConstructExpr>(VD->getInit());
  std::string exprStr;
  if (initListExpr) {
    exprStr = getExprStr(initListExpr, ASTCtx);
    exprStr = exprStr.substr(1, exprStr.size() - 2);
  } else if (cxxConstructExpr) {
    exprStr = getExprStr(cxxConstructExpr, ASTCtx);
    if (exprStr[0] == '{' && exprStr[exprStr.size() - 1] == '}') {
      exprStr = exprStr.substr(1, exprStr.size() - 2);
    }
  } else {
    exprStr = getExprStr(VD->getInit(), ASTCtx);
  }

  exprStr = "{" + exprStr + "}";
  unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
      DiagnosticIDs::Warning,
      "Braced-initialization {}, without equals sign, shall be "
      "used for variable initialization");

  std::string replacementStr = typeStr + " " + VD->getNameAsString() + exprStr;
  FixItHint hint =
      FixItHint::CreateReplacement(VD->getSourceRange(), replacementStr);
  DE.Report(VD->getLocation(), ID) << hint;
  return true;
}
