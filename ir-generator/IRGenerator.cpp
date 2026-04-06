#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <sstream>
#include <map>
#include <stack>
#include <cstdio>

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;

static llvm::cl::OptionCategory Assignment2Category("Assignment 2 Command Line Options");
static llvm::cl::opt<std::string>
    outFileName("o",
        llvm::cl::desc("Output file name"),
        llvm::cl::init("codegen.ir"),
        llvm::cl::cat(Assignment2Category));

// Helper: convert a Clang QualType to our IR type string
static std::string getTypeStr(QualType qt) {
    qt = qt.getUnqualifiedType();
    if (qt->isPointerType()) {
        QualType pointee = qt->getPointeeType().getUnqualifiedType();
        return getTypeStr(pointee) + " *";
    }
    if (qt->isFloatingType()) return "float";
    if (qt->isCharType())     return "char";
    if (qt->isVoidType())     return "void";
    return "int";
}

class Assignment2CodeGenerator : public ConstStmtVisitor<Assignment2CodeGenerator> {

    public:

        Assignment2CodeGenerator() : EC(), outFile_(outFileName, EC, llvm::sys::fs::OF_Text) {
            if (EC) {
                llvm::errs() << "Error: Could not write to file (" << EC.message() << ")\n";
                exit(0);
            }
        }

        void VisitFunctionDecl(FunctionDecl* funcDecl) {

            if(Stmt* body = funcDecl->getBody()) {

                // Generate code for the function signature
                QualType retType = funcDecl->getReturnType();
                std::string retTypeStr = getTypeStr(retType);
                outFile_ << "\ndefine " << retTypeStr << " @" << funcDecl->getNameAsString() << "(";
                for (unsigned i = 0; i < funcDecl->getNumParams(); ++i) {
                    if (i > 0) outFile_ << ", ";
                    ParmVarDecl* param = funcDecl->getParamDecl(i);
                    outFile_ << getTypeStr(param->getType()) << " %" << param->getNameAsString();
                }
                outFile_ << ") {\n\n";

                // Generate code for the function body
                // Do not forget to store parameters to the stack upon entry
                for (unsigned i = 0; i < funcDecl->getNumParams(); ++i) {
                    ParmVarDecl* param = funcDecl->getParamDecl(i);
                    unsigned reg = createReg(param);
                    std::string typeStr = getTypeStr(param->getType());
                    outFile_ << "%r" << reg << " = alloca " << typeStr << "\n";
                    outFile_ << "store " << typeStr << " %" << param->getNameAsString()
                             << ", " << typeStr << "* %r" << reg << "\n";
                }

                Visit(body);

                outFile_ << "\n}\n";

            }

        }

        void VisitCompoundStmt(const CompoundStmt* stmt) {

            // Generate code for compound statements (sequence of statements in curly braces)
            for (auto* s : stmt->body()) {
                Visit(s);
            }

        }

        void VisitDeclStmt(const DeclStmt* stmt) {

            for(auto decl = stmt->decl_begin(); decl != stmt->decl_end(); ++decl) {
                if(const VarDecl* vdecl = dyn_cast<VarDecl>(*decl)) {

                    // Generate code for variable declaration(s)
                    unsigned reg = createReg(vdecl);
                    std::string typeStr = getTypeStr(vdecl->getType());
                    outFile_ << "%r" << reg << " = alloca " << typeStr << "\n";
                    if (const Expr* init = vdecl->getInit()) {
                        Visit(init);
                        outFile_ << "store " << typeStr << " %r" << getReg(init)
                                 << ", " << typeStr << "* %r" << reg << "\n";
                    }

                }
            }

        }

        void VisitDeclRefExpr(const DeclRefExpr* E) {

            // You only need to support references to variables
            const VarDecl* vdecl = dyn_cast<VarDecl>(E->getDecl());
            assert(vdecl != NULL && "Unsupported reference");

            // Generate code for variable references
            // A DeclRefExpr is an lvalue — alias to the variable's alloca register
            setReg(E, getReg(vdecl));

        }

        void VisitIntegerLiteral(const IntegerLiteral *literal) {

            // Generate code for integer constants
            unsigned reg = createReg(literal);
            std::string typeStr = getTypeStr(literal->getType());
            outFile_ << "%r" << reg << " = seti " << typeStr << " "
                     << literal->getValue().getSExtValue() << "\n";

        }

        void VisitCharacterLiteral(const CharacterLiteral *literal) {

            // Generate code for character constants
            unsigned reg = createReg(literal);
            outFile_ << "%r" << reg << " = seti char " << literal->getValue() << "\n";

        }

        void VisitFloatingLiteral(const FloatingLiteral *literal) {

            // Generate code for float constants
            unsigned reg = createReg(literal);
            std::string typeStr = getTypeStr(literal->getType());
            double val = literal->getValue().convertToDouble();
            char buf[64];
            snprintf(buf, sizeof(buf), "%e", val);
            outFile_ << "%r" << reg << " = seti " << typeStr << " " << buf << "\n";

        }

        void VisitImplicitCastExpr(const ImplicitCastExpr* cast) {

            if(cast->getCastKind() != CK_LValueToRValue) {

                // You do not need to support implicit type casts, just lvalue to rvalue casts
                const Expr* sub = cast->getSubExpr();
                Visit(sub);
                setReg(cast, getReg(sub));

            } else {

                // Generate code for lvalue to rvalue casts
                const Expr* sub = cast->getSubExpr();
                Visit(sub);
                std::string typeStr    = getTypeStr(cast->getType());
                std::string ptrTypeStr = getTypeStr(sub->getType());
                unsigned reg = createReg(cast);
                outFile_ << "%r" << reg << " = load " << typeStr << ", "
                         << ptrTypeStr << "* %r" << getReg(sub) << "\n";

            }

        }

        void VisitBinaryOperator(const BinaryOperator *BO) {

            BinaryOperator::Opcode op = BO->getOpcode();

            // You do not need to support pointer-to-member binary operators (.*, ->*)
            assert(op != BO_PtrMemI && op != BO_PtrMemD && "Pointer-to-member operators not supported");

            if(op >= BO_Mul && op <= BO_Or) {

                // Generate code for arithmetic, logical, and relational operators
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();
                Visit(lhs);
                Visit(rhs);
                std::string typeStr = getTypeStr(lhs->getType());
                std::string opStr   = BO->getOpcodeStr().str();
                unsigned reg = createReg(BO);
                outFile_ << "%r" << reg << " = " << typeStr << " %r" << getReg(lhs)
                         << " " << opStr << " " << typeStr << " %r" << getReg(rhs) << "\n";

            } else if(op == BO_Assign) {

                // Generate code for the assignment operator
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();
                Visit(lhs);
                Visit(rhs);
                std::string typeStr = getTypeStr(rhs->getType());
                outFile_ << "store " << typeStr << " %r" << getReg(rhs)
                         << ", " << typeStr << "* %r" << getReg(lhs) << "\n";
                setReg(BO, getReg(lhs));

            } else if(op >= BO_MulAssign && op <= BO_OrAssign) {

                // Generate code for compound assignment operators
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();

                // Visit rhs first, then lhs, then load lhs value
                Visit(rhs);  // 1. evaluate rhs first
                Visit(lhs);  // 2. get lhs address

                std::string typeStr = getTypeStr(lhs->getType());

                // 3. load current lhs value
                unsigned loadReg = createReg();
                outFile_ << "%r" << loadReg << " = load " << typeStr << ", "
                         << typeStr << "* %r" << getReg(lhs) << "\n";

                std::string opStr;
                switch(op) {
                    case BO_MulAssign: opStr = "*";  break;
                    case BO_DivAssign: opStr = "/";  break;
                    case BO_RemAssign: opStr = "%";  break;
                    case BO_AddAssign: opStr = "+";  break;
                    case BO_SubAssign: opStr = "-";  break;
                    case BO_ShlAssign: opStr = "<<"; break;
                    case BO_ShrAssign: opStr = ">>"; break;
                    case BO_AndAssign: opStr = "&";  break;
                    case BO_XorAssign: opStr = "^";  break;
                    case BO_OrAssign:  opStr = "|";  break;
                    default: assert(0 && "Unknown compound assign");
                }

                // 4. compute result
                unsigned resReg = createReg();
                outFile_ << "%r" << resReg << " = " << typeStr << " %r" << loadReg
                         << " " << opStr << " " << typeStr << " %r" << getReg(rhs) << "\n";

                // 5. store result back
                outFile_ << "store " << typeStr << " %r" << resReg
                         << ", " << typeStr << "* %r" << getReg(lhs) << "\n";

                setReg(BO, getReg(lhs));

            } else if(op == BO_LAnd || op == BO_LOr) {

                // Generate code for short-circuiting Boolean expressions
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();

                unsigned lNext = createLabel();
                unsigned lExit = createLabel();

                Visit(lhs);
                unsigned lhsReg = getReg(lhs);

                if (op == BO_LAnd) {
                    // if lhs true → evaluate rhs; else → short-circuit
                    outFile_ << "br %r" << lhsReg << ", label L" << lNext
                             << ", label L" << lExit << "\n\n";
                } else {
                    // if lhs true → short-circuit; else → evaluate rhs
                    outFile_ << "br %r" << lhsReg << ", label L" << lExit
                             << ", label L" << lNext << "\n\n";
                }

                outFile_ << "L" << lNext << ":\n";
                Visit(rhs);
                unsigned rhsReg = getReg(rhs);
                outFile_ << "br label L" << lExit << "\n\n";

                outFile_ << "L" << lExit << ":\n";
                unsigned phiReg = createReg(BO);
                outFile_ << "%r" << phiReg << " = phi(%r" << lhsReg
                         << ", %r" << rhsReg << ")\n";

            } else {

                assert(0 && "Unsupported binary operator!");

            }

        }

        void VisitUnaryOperator(const UnaryOperator* UO) {

            UnaryOperator::Opcode op = UO->getOpcode();

            if(op >= UO_Plus && op <= UO_LNot) {

                // Generate code for unary arithmetic, logical, and Boolean operators
                const Expr* sub = UO->getSubExpr();
                Visit(sub);
                std::string typeStr = getTypeStr(UO->getType());
                std::string opStr   = UnaryOperator::getOpcodeStr(op).str();
                unsigned reg = createReg(UO);
                outFile_ << "%r" << reg << " = " << opStr << " "
                         << typeStr << " %r" << getReg(sub) << "\n";

            } else if(op == UO_PostInc || op == UO_PostDec) {

                // Generate code for post-fix operators (e.g., x++, x--)
                const Expr* sub = UO->getSubExpr();
                Visit(sub);
                std::string typeStr = getTypeStr(UO->getType());

                // load old value
                unsigned oldReg = createReg();
                outFile_ << "%r" << oldReg << " = load " << typeStr << ", "
                         << typeStr << "* %r" << getReg(sub) << "\n";

                // compute new value
                unsigned oneReg = createReg();
                outFile_ << "%r" << oneReg << " = seti " << typeStr << " 1\n";
                std::string opStr = (op == UO_PostInc) ? "+" : "-";
                unsigned newReg = createReg();
                outFile_ << "%r" << newReg << " = " << typeStr << " %r" << oldReg
                         << " " << opStr << " " << typeStr << " %r" << oneReg << "\n";

                // store new value back
                outFile_ << "store " << typeStr << " %r" << newReg
                         << ", " << typeStr << "* %r" << getReg(sub) << "\n";

                // result is old value
                setReg(UO, oldReg);

            } else if(op == UO_PreInc || op == UO_PreDec) {

                // Generate code for pre-fix operators (e.g., ++x, --x)
                const Expr* sub = UO->getSubExpr();
                Visit(sub);
                std::string typeStr = getTypeStr(UO->getType());

                // load current value
                unsigned curReg = createReg();
                outFile_ << "%r" << curReg << " = load " << typeStr << ", "
                         << typeStr << "* %r" << getReg(sub) << "\n";

                // compute new value
                unsigned oneReg = createReg();
                outFile_ << "%r" << oneReg << " = seti " << typeStr << " 1\n";
                std::string opStr = (op == UO_PreInc) ? "+" : "-";
                unsigned newReg = createReg();
                outFile_ << "%r" << newReg << " = " << typeStr << " %r" << curReg
                         << " " << opStr << " " << typeStr << " %r" << oneReg << "\n";

                // store new value back
                outFile_ << "store " << typeStr << " %r" << newReg
                         << ", " << typeStr << "* %r" << getReg(sub) << "\n";

                // result is lvalue address (so (++y) = 2 works)
                setReg(UO, getReg(sub));

            } else if(op == UO_AddrOf) {

                // Generate code for address-of operator (e.g., &x)
                const Expr* sub = UO->getSubExpr();
                Visit(sub);
                setReg(UO, getReg(sub));

            } else if(op == UO_Deref) {

                // Generate code for dereference operator (e.g., *x)
                const Expr* sub = UO->getSubExpr();
                Visit(sub);
                setReg(UO, getReg(sub));

            } else {

                assert(0 && "Unsupported unary operator!");

            }
        }

        void VisitCallExpr(const CallExpr* call) {

            // Generate code for call expressions
            for (unsigned i = 0; i < call->getNumArgs(); ++i) {
                Visit(call->getArg(i));
            }
            std::string args;
            for (unsigned i = 0; i < call->getNumArgs(); ++i) {
                if (i > 0) args += ", ";
                const Expr* arg = call->getArg(i);
                args += getTypeStr(arg->getType()) + " %r" + std::to_string(getReg(arg));
            }
            const FunctionDecl* callee = call->getDirectCallee();
            assert(callee && "Indirect calls not supported");
            std::string calleeName  = callee->getNameAsString();
            std::string retTypeStr  = getTypeStr(call->getType());
            unsigned reg = createReg(call);
            outFile_ << "%r" << reg << " = call " << retTypeStr
                     << " @" << calleeName << "(" << args << ")\n";

        }

        void VisitReturnStmt(const ReturnStmt* Stmt) {

            // Generate code for return statements
            if (const Expr* retVal = Stmt->getRetValue()) {
                Visit(retVal);
                std::string typeStr = getTypeStr(retVal->getType());
                outFile_ << "ret " << typeStr << " %r" << getReg(retVal) << "\n";
            } else {
                outFile_ << "ret void\n";
            }

        }

        void VisitParenExpr(const ParenExpr* expr) {

            // Generate code for expressions in parentheses
            const Expr* sub = expr->getSubExpr();
            Visit(sub);
            setReg(expr, getReg(sub));

        }

        void VisitNullStmt(const NullStmt* stmt) {

            // Generate code for empty statement — nothing to emit

        }

        void VisitIfStmt(const IfStmt* ifStmt) {

            // Generate code for if statement
            unsigned lThen = createLabel();
            unsigned lElse = createLabel();
            unsigned lExit = createLabel();

            const Expr* cond = ifStmt->getCond();
            Visit(cond);

            if (ifStmt->getElse()) {
                outFile_ << "br %r" << getReg(cond) << ", label L" << lThen
                         << ", label L" << lElse << "\n\n";
                outFile_ << "L" << lThen << ":\n";
                Visit(ifStmt->getThen());
                outFile_ << "br label L" << lExit << "\n\n";
                outFile_ << "L" << lElse << ":\n";
                Visit(ifStmt->getElse());
                outFile_ << "br label L" << lExit << "\n\n";
                outFile_ << "L" << lExit << ":\n";
            } else {
                outFile_ << "br %r" << getReg(cond) << ", label L" << lThen
                         << ", label L" << lExit << "\n\n";
                outFile_ << "L" << lThen << ":\n";
                Visit(ifStmt->getThen());
                outFile_ << "br label L" << lExit << "\n\n";
                outFile_ << "L" << lExit << ":\n";
            }

        }

        void VisitConditionalOperator(const ConditionalOperator* CO) {

            // Generate code for ternary operator
            unsigned lTrue  = createLabel();
            unsigned lFalse = createLabel();
            unsigned lExit  = createLabel();

            const Expr* cond  = CO->getCond();
            const Expr* tExpr = CO->getTrueExpr();
            const Expr* fExpr = CO->getFalseExpr();

            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << lTrue
                     << ", label L" << lFalse << "\n\n";

            outFile_ << "L" << lTrue << ":\n";
            Visit(tExpr);
            outFile_ << "br label L" << lExit << "\n\n";

            outFile_ << "L" << lFalse << ":\n";
            Visit(fExpr);
            outFile_ << "br label L" << lExit << "\n\n";

            outFile_ << "L" << lExit << ":\n";
            unsigned phiReg = createReg(CO);
            outFile_ << "%r" << phiReg << " = phi(%r" << getReg(tExpr)
                     << ", %r" << getReg(fExpr) << ")\n";

        }

        void VisitWhileStmt(const WhileStmt* whileStmt) {

            // Generate code for while loop
            unsigned lCond = createLabel();
            unsigned lBody = createLabel();
            unsigned lExit = createLabel();

            pushBreakLabel(lExit);
            pushContinueLabel(lCond);

            outFile_ << "\nL" << lCond << ":\n";
            const Expr* cond = whileStmt->getCond();
            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << lBody
                     << ", label L" << lExit << "\n\n";

            outFile_ << "L" << lBody << ":\n";
            Visit(whileStmt->getBody());
            outFile_ << "br label L" << lCond << "\n\n";

            outFile_ << "L" << lExit << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitDoStmt(const DoStmt* doStmt) {

            // Generate code for do-while loop
            unsigned lBody = createLabel();
            unsigned lCond = createLabel();
            unsigned lExit = createLabel();

            pushBreakLabel(lExit);
            pushContinueLabel(lCond);

            outFile_ << "\nL" << lBody << ":\n";
            Visit(doStmt->getBody());
            outFile_ << "br label L" << lCond << "\n\n";

            outFile_ << "L" << lCond << ":\n";
            const Expr* cond = doStmt->getCond();
            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << lBody
                     << ", label L" << lExit << "\n\n";

            outFile_ << "L" << lExit << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitForStmt(const ForStmt* forStmt) {

            // Generate code for for loop
            unsigned lCond = createLabel();
            unsigned lBody = createLabel();
            unsigned lPost = createLabel();
            unsigned lExit = createLabel();

            pushBreakLabel(lExit);
            pushContinueLabel(lPost);

            // init
            if (const Stmt* init = forStmt->getInit()) {
                Visit(init);
            }
            outFile_ << "br label L" << lCond << "\n\n";

            // condition
            outFile_ << "L" << lCond << ":\n";
            if (const Expr* cond = forStmt->getCond()) {
                Visit(cond);
                outFile_ << "br %r" << getReg(cond) << ", label L" << lBody
                         << ", label L" << lExit << "\n\n";
            } else {
                outFile_ << "br label L" << lBody << "\n\n";
            }

            // body
            outFile_ << "L" << lBody << ":\n";
            Visit(forStmt->getBody());
            outFile_ << "br label L" << lPost << "\n\n";

            // post
            outFile_ << "L" << lPost << ":\n";
            if (const Expr* inc = forStmt->getInc()) {
                Visit(inc);
            }
            outFile_ << "br label L" << lCond << "\n\n";

            // exit
            outFile_ << "L" << lExit << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitBreakStmt(const BreakStmt* B) {

            // Generate code for break statement
            outFile_ << "br label L" << getBreakLabel() << "\n";

        }

        void VisitContinueStmt(const ContinueStmt* C) {

            // Generate code for continue statement
            outFile_ << "br label L" << getContinueLabel() << "\n";

        }

        void VisitArraySubscriptExpr(const ArraySubscriptExpr* E) {
            // You do not need to support arrays
            assert(0 && "Arrays not supported");
        }

        void VisitSwitchStmt(const SwitchStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitCaseStmt(const CaseStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitDefaultStmt(const DefaultStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitStringLiteral(const StringLiteral *literal) {
            // You do not need to support string literals
            assert(0 && "String literals not supported");
        }

        void VisitMemberExpr(const MemberExpr* expr) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitCompoundLiteralExpr(const CompoundLiteralExpr *E) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitInitListExpr(const InitListExpr* E) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitStmt(const Stmt* stmt) {
            // You do not need to support any other type of statement
            assert(0 && "Unsupported statement!");
        }

    private:

        std::error_code EC;
        llvm::raw_fd_ostream outFile_;

        // Used to generate unique virtual registers
        unsigned int regGenerator = 0;
        std::map<const VarDecl*,unsigned int> vdecl2reg_;
        std::map<const Expr*,unsigned int> expr2reg_;

        // Create a unique virtual register for a local variable
        unsigned int createReg(const VarDecl* vdecl) {
            assert(!vdecl2reg_.count(vdecl));
            vdecl2reg_[vdecl] = regGenerator++;
            return vdecl2reg_[vdecl];
        }

        // Create a unique virtual register for the temporary result of an expression evaluation
        unsigned int createReg(const Expr* expr) {
            assert(!expr2reg_.count(expr));
            expr2reg_[expr] = regGenerator++;
            return expr2reg_[expr];
        }

        // Create a unique virtual register for a temporary result that is not the direct result of an expression evaluation
        unsigned int createReg() {
            return regGenerator++;
        }

        // Get the unique virtual register for a local variable
        unsigned int getReg(const VarDecl* vdecl) {
            assert(vdecl2reg_.count(vdecl));
            return vdecl2reg_[vdecl];
        }

        // Get the unique virtual register for the temporary result of an expression evaluation
        unsigned int getReg(const Expr* expr) {
            assert(expr2reg_.count(expr));
            return expr2reg_[expr];
        }

        // Set the virtual register of an expression evaluation to some existing register
        void setReg(const Expr* expr, unsigned int reg) {
            assert(!expr2reg_.count(expr));
            expr2reg_[expr] = reg;
        }

        // Used to generate labels and track the destination of break and continue
        unsigned int labelGenerator = 0;
        std::stack<unsigned int> breakLabels_;
        std::stack<unsigned int> continueLabels_;

        // Create a unique label
        unsigned int createLabel() { return labelGenerator++; }

        // Manipulate the break destination stack
        void pushBreakLabel(unsigned int label) { breakLabels_.push(label); }
        void popBreakLabel() { breakLabels_.pop(); }
        unsigned int getBreakLabel() { return breakLabels_.top(); }

        // Manipulate the continue destination stack
        void pushContinueLabel(unsigned int label) { continueLabels_.push(label); }
        void popContinueLabel() { continueLabels_.pop(); }
        unsigned int getContinueLabel() { return continueLabels_.top(); }

};

class Assignment2Visitor : public RecursiveASTVisitor<Assignment2Visitor> {

    public:

        Assignment2Visitor() { }

        bool VisitFunctionDecl(FunctionDecl *funcDecl) {
            codeGenerator_.VisitFunctionDecl(funcDecl);
            return true;
        }

    private:

        Assignment2CodeGenerator codeGenerator_;

};

class Assignment2ASTConsumer : public ASTConsumer {

    public:

        Assignment2ASTConsumer() {}

        virtual void HandleTranslationUnit(ASTContext &Context) override {
            TranslationUnitDecl* TU = Context.getTranslationUnitDecl();
            Assignment2Visitor visitor;
            visitor.TraverseDecl(TU);
        }

};

class Assignment2Action : public ASTFrontendAction {

    public:

        Assignment2Action() {}

        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            return std::make_unique<Assignment2ASTConsumer>();
        }

};

int main(int argc, const char **argv) {
    auto ExpectedParser = tooling::CommonOptionsParser::create(argc, argv, Assignment2Category);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<Assignment2Action>().get());
}