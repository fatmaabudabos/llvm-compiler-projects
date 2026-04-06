#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <string>
#include <sstream>

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;

class Assignment1Transform : public RecursiveASTVisitor<Assignment1Transform> {

    public:

        Assignment1Transform(Rewriter& rewriter) : rewriter_(rewriter) { }

        bool VisitFunctionDecl(FunctionDecl *funcDecl) {

            // If the function is main and it has a body, insert code at the beginning of the function that declares and initializes the controller

            // TODO
            if (funcDecl->isMain() && funcDecl->hasBody()) {
                Stmt* body = funcDecl->getBody();
                if (CompoundStmt* compStmt = dyn_cast<CompoundStmt>(body)) {
                    SourceLocation startLoc = compStmt->getLBracLoc().getLocWithOffset(1);
                    std::string controllerInit = "\n    struct Controller controller;\n";
                    controllerInit += "    controller.skipIf = 0;\n";
                    controllerInit += "    controller.skipElse = 0;\n";
                    controllerInit += "    controller.skipWhile = 0;\n";
                    controllerInit += "    controller.skipDoWhile = 0;\n";
                    controllerInit += "    controller.skipFor = 0;\n";
                    controllerInit += "    controller.skipBreak = 0;\n";
                    controllerInit += "    controller.skipContinue = 0;\n";
                    controllerInit += "    controller.skipFunctionName[0] = '\\0';\n";
                    rewriter_.InsertTextAfterToken(startLoc, controllerInit);
                }
            }

            // If the function is not main, insert a parameter and the end that takes the controller
            // Assume that the function already has at least one parameter

            // TODO
            if (!funcDecl->isMain() && funcDecl->hasBody() && funcDecl->param_size() > 0) {
                ParmVarDecl* lastParam = funcDecl->getParamDecl(funcDecl->param_size() - 1);
                SourceLocation lastParamEnd = lastParam->getEndLoc();
                std::string controllerParam = ", struct Controller controller";
                rewriter_.InsertTextAfterToken(lastParamEnd, controllerParam);
            }

            return true;

        }

        bool VisitIfStmt(IfStmt* ifStmt) {

            // Modify the condition to also check whether or not to skip the then statement

            // TODO
            Expr* cond = ifStmt->getCond();
            SourceLocation condBegin = cond->getBeginLoc();
            SourceLocation condEnd = cond->getEndLoc();
            
            rewriter_.InsertTextBefore(condBegin, "(");
            rewriter_.InsertTextAfterToken(condEnd, ") && !controller.skipIf");

            // If an else statement exist, guard it with an if statement that checks whether or not to skip else

            // TODO
            if (ifStmt->getElse() != nullptr) {
                Stmt* elseStmt = ifStmt->getElse();
                SourceLocation elseBegin = elseStmt->getBeginLoc();
                
                rewriter_.InsertTextBefore(elseBegin, "if(!controller.skipElse) ");
            }

            return true;

        }

        bool VisitWhileStmt(WhileStmt* whileStmt) {

            // Modify the condition to also check whether or not to skip while loops

            // TODO
            Expr* cond = whileStmt->getCond();
            SourceLocation condBegin = cond->getBeginLoc();
            SourceLocation condEnd = cond->getEndLoc();
            
            rewriter_.InsertTextBefore(condBegin, "(");
            rewriter_.InsertTextAfterToken(condEnd, ") && !controller.skipWhile");

            return true;

        }

        bool VisitDoStmt(DoStmt* doStmt) {

            // Guard the do-while loop with an if statement that checks whether or not skip do-while loops

            // TODO
            SourceLocation doBegin = doStmt->getBeginLoc();
            rewriter_.InsertTextBefore(doBegin, "if(!controller.skipDoWhile) ");

            return true;

        }

        bool VisitForStmt(ForStmt* forStmt) {

            // Modify the condition to also check whether or not to skip for loops

            // TODO
            Expr* cond = forStmt->getCond();
            if (cond != nullptr) {
                SourceLocation condBegin = cond->getBeginLoc();
                SourceLocation condEnd = cond->getEndLoc();
                
                rewriter_.InsertTextBefore(condBegin, "(");
                rewriter_.InsertTextAfterToken(condEnd, ") && !controller.skipFor");
            }

            return true;

        }

        bool VisitBreakStmt(BreakStmt* breakStmt) {

            // Guard the break statement with an if statement the checks whether or not to skip break statements

            // TODO
            SourceLocation breakBegin = breakStmt->getBeginLoc();
            rewriter_.InsertTextBefore(breakBegin, "if(!controller.skipBreak) ");

            return true;
        }

        bool VisitContinueStmt(ContinueStmt* continueStmt) {

            // Guard the continue statement with an if statement the checks whether or not to skip continue statements

            // TODO
            SourceLocation continueBegin = continueStmt->getBeginLoc();
            rewriter_.InsertTextBefore(continueBegin, "if(!controller.skipContinue) ");

            return true;
        }

        bool VisitCallExpr(CallExpr* callExpr) {

            // Add the controller as an argument at the end
            // Assume a functions is called directly
            // Assume that the function called already has at least one argument

            // TODO
            if (callExpr->getDirectCallee() != nullptr && callExpr->getNumArgs() > 0) {
                FunctionDecl* callee = callExpr->getDirectCallee();
                // Only add controller if it's a user-defined function (has a body in this translation unit)
                if (callee->hasBody()) {
                    Expr* lastArg = callExpr->getArg(callExpr->getNumArgs() - 1);
                    SourceLocation lastArgEnd = lastArg->getEndLoc();
                    rewriter_.InsertTextAfterToken(lastArgEnd, ", controller");
                }
            }

            // Guard direct function calls with a check that skips calls to functions with a particular name
            // Assume a functions is called directly
            // Assume the call expression is its own statement, not a subexpression of another statement

            // TODO
            // REMOVED: This causes syntax errors when calls are inside expressions
            // We only add the controller argument above, but don't guard the calls

            return true;

        }

    private:

        Rewriter &rewriter_;

};

static llvm::cl::OptionCategory Assignment1Category("Assignment 1 Command Line Options");
static llvm::cl::opt<std::string>
    outFileName("o",
        llvm::cl::desc("Output file name"),
        llvm::cl::init("transformed.c"),
        llvm::cl::cat(Assignment1Category));

class Assignment1ASTConsumer : public ASTConsumer {

    public:

        Assignment1ASTConsumer(Rewriter &rewriter)
            : rewriter_(rewriter) {}

        virtual void HandleTranslationUnit(ASTContext &Context) {
            TranslationUnitDecl* TU = Context.getTranslationUnitDecl();
            Assignment1Transform transform(rewriter_);
            transform.TraverseDecl(TU);
        }

    private:

        Rewriter& rewriter_;

};

class Assignment1Action : public ASTFrontendAction {

    public:

        Assignment1Action() {}

        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
            return std::make_unique<Assignment1ASTConsumer>(rewriter_);
        }

        void EndSourceFileAction() override {
            std::error_code EC;
            llvm::sys::fs::OpenFlags flags = llvm::sys::fs::OF_Text;
            llvm::raw_fd_ostream FileStream(outFileName, EC, flags);
            if (EC) {
                llvm::errs() << "Error: Could not write to " << EC.message() << "\n";
            } else {

                // Add a declaration of the controller structure to the begining of the file

                // TODO
                std::string structDecl = "#include <string.h>\n\n";
                structDecl += "struct Controller {\n";
                structDecl += "    int skipIf;\n";
                structDecl += "    int skipElse;\n";
                structDecl += "    int skipWhile;\n";
                structDecl += "    int skipDoWhile;\n";
                structDecl += "    int skipFor;\n";
                structDecl += "    int skipBreak;\n";
                structDecl += "    int skipContinue;\n";
                structDecl += "    char skipFunctionName[100];\n";
                structDecl += "};\n\n";
                
                FileStream << structDecl;

                SourceManager &SM = rewriter_.getSourceMgr();
                rewriter_.getEditBuffer(SM.getMainFileID()).write(FileStream);

            }
        }

    private:
        Rewriter rewriter_;

};

int main(int argc, const char **argv) {
    auto ExpectedParser = tooling::CommonOptionsParser::create(argc, argv, Assignment1Category);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<Assignment1Action>().get());
}
