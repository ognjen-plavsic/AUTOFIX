#include "AutoFixMatchers.hpp"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "AutofixDiagnosticConsumer.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/DiagnosticOptions.h"

using namespace clang::tooling;

internal::Matcher<Decl> declMatcher =
    decl(isExpansionInMainFile()).bind("decl");

class AutoFixConsumer : public clang::ASTConsumer {
public:
  explicit AutoFixConsumer(ASTContext *Context, SourceManager &SM) : SM(SM) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    auto &DE = Context.getDiagnostics();
    auto &DO = DE.getDiagnosticOptions();
    DO.SnippetLineLimit = 10;

    DeclInit DPrinter(Context, SM);
    MatchFinder Finder;
    Finder.addMatcher(declMatcher, &DPrinter);
    Finder.matchAST(Context);
  }
  SourceManager &SM;
};

class AutoFixAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::make_unique<AutoFixConsumer>(&Compiler.getASTContext(),
                                             Compiler.getSourceManager());
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

static llvm::cl::OptionCategory AutoFixCategory("auto-fix options");

cl::opt<bool> ApplyFix("apply-fix", cl::desc(R"(Apply suggested fixes. )"),
                         cl::init(false), cl::cat(AutoFixCategory));

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");


int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, AutoFixCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();

  auto *DO = new DiagnosticOptions();
  DO->ShowColors = true;
  Rewriter Rewrite;
  AutoFixDiagnosticConsumer DiagConsumer(llvm::outs(), &*DO, Rewrite);

  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  Tool.setDiagnosticConsumer(&DiagConsumer);
  AutoFixActionFactory actionFactory;
  return Tool.run(&actionFactory);
}