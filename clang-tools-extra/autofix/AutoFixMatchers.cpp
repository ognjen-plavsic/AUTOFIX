#include "AutoFixMatchers.hpp"

void VarDeclInit::run(const MatchFinder::MatchResult &Result) {
  if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("varDecl")) {
    if (VD->hasInit()) {
      if (warnAutoTypeBracedInit(VD))
        return;
      warnNonAutoTypeBracedInit(VD);
    }
  }
}

bool VarDeclInit::warnAutoTypeBracedInit(const VarDecl *VD) {
  auto &DE = ASTCtx.getDiagnostics();
  std::string typeStr = VD->getType().getAsString();
  if (auto dty = llvm::dyn_cast<clang::AutoType>(VD->getType().getTypePtr())) {
    if (!dty->isDecltypeAuto()) {
      if (VD->getInitStyle() == VarDecl::ListInit) {

        auto listInitExpr = getInitListExpr(VD->getInit());
        assert(listInitExpr);
        unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Warning,
            "A variable of type auto shall not be initialized using {} "
            "or ={} braced initialization");
        std::string exprStr = getExprStr(listInitExpr, ASTCtx);
        exprStr = exprStr.substr(1, exprStr.size() - 2);
        exprStr = " = " + exprStr;

        std::string replacementStr =
            typeStr + " " + VD->getNameAsString() + exprStr;
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
  std::string exprStr = getExprStr(VD->getInit(), ASTCtx);
  if (!llvm::dyn_cast<InitListExpr>(VD->getInit())) {
    exprStr = "{" + exprStr + "}";
  }
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
