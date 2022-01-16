#include "AutoFixMatchers.hpp"
#include <iostream>
#include <set>

void DeclInit::emitWarningWithHint(std::string &msg,
                                   std::string &replacementStr, SourceRange SR,
                                   SourceLocation SL) {
  auto &DE = ASTCtx.getDiagnostics();
  unsigned ID =
      DE.getDiagnosticIDs()->getCustomDiagID(DiagnosticIDs::Warning, msg);
  FixItHint hint = FixItHint::CreateReplacement(SR, replacementStr);
  DE.Report(SL, ID) << hint;
}

bool DeclInit::warnAutoTypeBracedInit(const VarDecl *VD) {
  if (auto dty = llvm::dyn_cast<clang::AutoType>(VD->getType().getTypePtr())) {
    if (!dty->isDecltypeAuto()) {
      if (VD->getInitStyle() == VarDecl::ListInit ||
          VD->getInitStyle() == VarDecl::CInit) {
        auto listInitExpr = getChildOfType<InitListExpr>(VD->getInit());
        if (!listInitExpr) {
          return false;
        }
        std::string exprStr = getExprStr(listInitExpr, ASTCtx);
        exprStr = exprStr.substr(1, exprStr.size() - 2);
        exprStr = " = " + exprStr;

        std::string replacementStr = "auto " + VD->getNameAsString() + exprStr;
        std::string msg =
            "A variable of type auto shall not be initialized using {} "
            "or ={} braced initialization";
        emitWarningWithHint(msg, replacementStr, VD->getSourceRange(),
                            VD->getLocation());
        return true;
      }
    }
  }
  return false;
}

bool DeclInit::warnNonAutoTypeBracedInit(const VarDecl *VD) {
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
  std::string replacementStr = typeStr + " " + VD->getNameAsString() + exprStr;
  std::string msg = "Braced-initialization {}, without equals sign, shall be "
                    "used for variable initialization";
  emitWarningWithHint(msg, replacementStr, VD->getSourceRange(),
                      VD->getLocation());
  return true;
}

void DeclInit::checkWrongPlacedSpecifiers(std::string &typeStr,
                                          std::string &declString,
                                          const Decl *D) {
  std::vector<std::string> nonTypeSpecifiers{
      "typedef",      "friend",  "constexpr", "register", "static",  "extern",
      "thread_local", "mutable", "inline",    "virtual",  "explicit"};

  std::vector<std::string> typeIdsVec = getWordsFromString(typeStr);
  std::vector<std::string> declWordsVec = getWordsFromString(declString);
  for (auto nonTypeWord : nonTypeSpecifiers) {
    auto NTSIt =
        std::find(declWordsVec.begin(), declWordsVec.end(), nonTypeWord);
    if (NTSIt != declWordsVec.end()) {
      for (auto typeId : typeIdsVec) {
        auto TSIt = std::find(declWordsVec.begin(), declWordsVec.end(), typeId);
        if (TSIt != declWordsVec.end() && TSIt < NTSIt) {
          std::iter_swap(NTSIt, TSIt);
          // Update iterator for non-type specifier since its place in vector
          // changed.
          NTSIt =
              std::find(declWordsVec.begin(), declWordsVec.end(), nonTypeWord);

          // create replacement string by concatenating words from vector
          std::string replacementStr = declWordsVec[0];
          for (auto i = 1; i < declWordsVec.size(); i++) {
            replacementStr += " " + declWordsVec[i];
          }
          std::string warningMsg =
              "A non-type specifier shall be placed before a "
              "type specifier in a declaration";
          emitWarningWithHint(warningMsg, replacementStr, D->getSourceRange(),
                              D->getLocation());
        }
      }
    }
  }
}

void DeclInit::warnWrongPlacedSpecifiers(const Decl *D) {
  std::string declString;
  std::string typeStr;
  if (const ValueDecl *VD = llvm::dyn_cast<clang::ValueDecl>(D)) {
    declString = getSourceString(SM, VD->getBeginLoc(), VD->getEndLoc());
    typeStr = VD->getType().getAsString();
    checkWrongPlacedSpecifiers(typeStr, declString, D);
  } else if (const TypedefDecl *TD = llvm::dyn_cast<clang::TypedefDecl>(D)) {
    auto name = TD->getName().str();
    declString =
        getSourceString(SM, TD->getBeginLoc(), TD->getEndLoc(), name.length());
    typeStr = TD->getUnderlyingType().getAsString();
    checkWrongPlacedSpecifiers(typeStr, declString, D);
  }
}

void DeclInit::run(const MatchFinder::MatchResult &Result) {
  auto D = Result.Nodes.getNodeAs<clang::Decl>("decl");
  warnWrongPlacedSpecifiers(D);

  if (const TypedefDecl *TD = llvm::dyn_cast<clang::TypedefDecl>(D)) {
    auto typeStr = TD->getUnderlyingType().getAsString();
    std::string replacementStr =
        "using " + TD->getName().str() + " = " + typeStr;

    std::string msg = "The typedef specifier shall not be used.";
    emitWarningWithHint(msg, replacementStr, TD->getSourceRange(),
                        TD->getLocation());
  }

  if (const VarDecl *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
    if (VD->hasInit()) {
      warnAutoTypeBracedInit(VD);
      warnNonAutoTypeBracedInit(VD);
    }
  }
}
