#include "AutoFixMatchers.hpp"
#include <iostream>
#include <set>
#include <sstream>

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

void TypedefDeclInit::run(const MatchFinder::MatchResult &Result) {
  if (const TypedefDecl *TD =
          Result.Nodes.getNodeAs<clang::TypedefDecl>("typedefDecl")) {
    auto &DE = ASTCtx.getDiagnostics();

    unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
        DiagnosticIDs::Warning, "The typedef specifier shall not be used.");

    auto typeStr = TD->getUnderlyingType().getAsString();
    std::string replacementStr =
        "using " + TD->getName().str() + " = " + typeStr;
    FixItHint hint =
        FixItHint::CreateReplacement(TD->getSourceRange(), replacementStr);
    DE.Report(TD->getLocation(), ID) << hint;
  }
}

void DeclInit::run(const MatchFinder::MatchResult &Result) {
  auto D = Result.Nodes.getNodeAs<clang::Decl>("decl");
  auto beginPtr = SM.getCharacterData(D->getBeginLoc());
  auto endPtr = SM.getCharacterData(D->getEndLoc());
  std::string declString(beginPtr, endPtr - beginPtr + 1);

  std::vector<std::string> nonTypeSpecifiers{
      "typedef",      "friend",  "constexpr", "register", "static",  "extern",
      "thread_local", "mutable", "inline",    "virtual",  "explicit"};

  if (const ValueDecl *VD = llvm::dyn_cast<clang::ValueDecl>(D)) {
    std::string typeStr = VD->getType().getAsString();
    std::vector<std::string> typeIdsVec;
    std::vector<std::string> declWordsVec;

    std::istringstream typeStream(typeStr);
    std::istringstream declStringStream(declString);
    std::string typeWord;
    while (getline(typeStream, typeWord, ' ')) {
      typeIdsVec.push_back(typeWord);
    }

    std::string declWord;
    while (getline(declStringStream, declWord, ' ')) {
      declWordsVec.push_back(declWord);
    }

    for (auto nonTypeWord : nonTypeSpecifiers) {
      auto NTSIt =
          std::find(declWordsVec.begin(), declWordsVec.end(), nonTypeWord);
      if (NTSIt != declWordsVec.end()) {
        for (auto typeId : typeIdsVec) {
          auto TSIt =
              std::find(declWordsVec.begin(), declWordsVec.end(), typeId);
          if (TSIt != declWordsVec.end() && TSIt < NTSIt) {
            std::iter_swap(NTSIt, TSIt);
          }
        }
      }
    }
    std::string replacementStr = declWordsVec[0];
    for (auto i = 1; i < declWordsVec.size(); i++) {
      replacementStr += " " + declWordsVec[i];
    }

    auto &DE = ASTCtx.getDiagnostics();
    unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
        DiagnosticIDs::Warning, "A non-type specifier shall be placed before a "
                                "type specifier in a declaration");
    FixItHint hint =
        FixItHint::CreateReplacement(VD->getSourceRange(), replacementStr);
    DE.Report(VD->getLocation(), ID) << hint;
  }
}
