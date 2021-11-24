#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include <iostream>

using namespace clang::tooling;
using namespace llvm;

using namespace clang;
using namespace clang::ast_matchers;

internal::Matcher<Decl> varDeclMatcher =
    varDecl(isExpansionInMainFile()).bind("varDecl");

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

class VarDeclInit : public MatchFinder::MatchCallback {
public:
  VarDeclInit(ASTContext &ASTCtx) : ASTCtx(ASTCtx) {}
  virtual void run(const MatchFinder::MatchResult &Result) {

    if (const VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("varDecl")) {
      if (VD->hasInit() && VD->getInitStyle() != VarDecl::ListInit) {
        auto &DE = ASTCtx.getDiagnostics();
        unsigned ID = DE.getDiagnosticIDs()->getCustomDiagID(
            DiagnosticIDs::Warning,
            "Braced-initialization {}, without equals sign, shall be "
            "used for variable initialization");

        std::string exprStr = getExprStr(VD->getInit(), ASTCtx);
        std::string typeStr = VD->getType().getAsString();
        if (!llvm::dyn_cast<InitListExpr>(VD->getInit())) {
          exprStr = "{" + exprStr + "}";
        }
        std::string replacementStr =
            typeStr + " " + VD->getNameAsString() + exprStr;
        FixItHint hint =
            FixItHint::CreateReplacement(VD->getSourceRange(), replacementStr);
        DE.Report(VD->getLocation(), ID) << hint;
      }
    }
  }

private:
  ASTContext &ASTCtx;
};

class AutoFixConsumer : public clang::ASTConsumer {
public:
  explicit AutoFixConsumer(ASTContext *Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    VarDeclInit Printer(Context);
    MatchFinder Finder;
    Finder.addMatcher(varDeclMatcher, &Printer);
    Finder.matchAST(Context);
  }
};

class AutoFixAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::make_unique<AutoFixConsumer>(&Compiler.getASTContext());
  }
};

class AutoFixActionFactory : public FrontendActionFactory {
public:
  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<AutoFixAction>();
  }

  bool runInvocation(std::shared_ptr<CompilerInvocation> Invocation,
                     FileManager *Files,
                     std::shared_ptr<PCHContainerOperations> PCHContainerOps,
                     DiagnosticConsumer *DiagConsumer) override {
    return FrontendActionFactory::runInvocation(Invocation, Files,
                                                PCHContainerOps, DiagConsumer);
  }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  AutoFixActionFactory actionFactory;
  return Tool.run(&actionFactory);
}