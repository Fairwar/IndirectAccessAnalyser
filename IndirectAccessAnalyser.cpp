#include "clang/AST/APValue.h"
#include "clang/AST/ASTFwd.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/StmtIterator.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <map>
#include <memory>

using namespace clang;

class FindIndirectAccessVisitor
    : public RecursiveASTVisitor<FindIndirectAccessVisitor> {

public:
  explicit FindIndirectAccessVisitor(ASTContext *Context)
      : Context(Context), SM(Context->getSourceManager()) {}

  // 在此处主要重写方法 Traverse
  // 重写 Traverse 类的方法可以控制遍历顺序
  // 并写一个访问函数, 在访问到某个节点时, 调用该函数

  // 对于各类节点都有对应的 Traverse 方法
  // 如对于 DeclRefExpr 节点, 有: TraverseDeclRefExpr

  // 下面举例一个后根遍历 Stmt
  // 并在访问到 Expr 时, 打印表达式的信息的例子

  // WRITE CODE FROM HERE
  bool TraverseStmt(Stmt *s) {
    if (s) {
      // 递归遍历子节点
      for (Stmt *Child : s->children()) {
        TraverseStmt(Child);
      }

      // 处理变量声明语句节点
      if (isa<DeclStmt>(s)) {
        for (DeclStmt::decl_iterator it = cast<DeclStmt>(s)->decl_begin();
             it != cast<DeclStmt>(s)->decl_end(); it++) {
          
          // 处理变量声明节点
          if (isa<VarDecl>(*it)) {
            Expr *init = cast<VarDecl>(*it)->getInit();

            // 如果有初始化表达式
            if (init != nullptr) {
              VarIndirectAccessCount[cast<VarDecl>(*it)] = ExprIndirectAccessCount[init];
            }

            // 如果没有初始化表达式
            else{
              VarIndirectAccessCount[cast<VarDecl>(*it)] = 0;
            }
          }
        }
      }

      // 处理表达式节点
      else if (isa<Expr>(s)) {
        ExprIndirectAccessCount[cast<Expr>(s)] = HandleExpr(cast<Expr>(s));
        PrintExpr(cast<Expr>(s));
      }
    }
    return true;
  }

private:
  int HandleExpr(Expr *e) {

    int ret = 0;
    // 常数节点
    if (isa<IntegerLiteral>(e)) {
      ret = 0;
    }

    // 变量引用节点
    else if (isa<DeclRefExpr>(e)) {
      if (isa<VarDecl>(cast<DeclRefExpr>(e)->getDecl())) {
        ret = VarIndirectAccessCount[cast<VarDecl>(
            cast<DeclRefExpr>(e)->getDecl())];
      }
    }

    // 数组访问节点
    else if (isa<ArraySubscriptExpr>(e)) {
      ret = ExprIndirectAccessCount[cast<ArraySubscriptExpr>(e)->getIdx()] + 1;
    }

    // 类型转换节点
    else if (isa<ImplicitCastExpr>(e)) {
      ret = ExprIndirectAccessCount[cast<ImplicitCastExpr>(e)->getSubExpr()];
    }

    // 二元运算节点
    else if (isa<BinaryOperator>(e)) {

      // 赋值运算节点
      if (cast<BinaryOperator>(e)->isAssignmentOp()) {
        // 左值为变量
        if (isa<DeclRefExpr>(cast<BinaryOperator>(e)->getLHS())) {
          if (isa<VarDecl>(cast<DeclRefExpr>(cast<BinaryOperator>(e)->getLHS())
                               ->getDecl())) {
            VarIndirectAccessCount[cast<VarDecl>(
                cast<DeclRefExpr>(cast<BinaryOperator>(e)->getLHS())
                    ->getDecl())] =
                ExprIndirectAccessCount[cast<BinaryOperator>(e)->getRHS()];
            ret = ExprIndirectAccessCount[cast<BinaryOperator>(e)->getRHS()];
          }
        }
        
      }
      // 其他二元运算节点
      else {
        ret = std::max(
            ExprIndirectAccessCount[cast<BinaryOperator>(e)->getLHS()],
            ExprIndirectAccessCount[cast<BinaryOperator>(e)->getRHS()]);
      }
    }

    // 一元运算节点
    else if (isa<UnaryOperator>(e)) {
      ret = ExprIndirectAccessCount[cast<UnaryOperator>(e)->getSubExpr()];
    }

    // PrintExpr(e);

    return ret;
  }
  // WRITE CODE END HERE

  void PrintExpr(Expr *e) {
    SourceLocation Loc = e->getExprLoc();
    if (Loc.isValid()) {
      FullSourceLoc FullLoc = Context->getFullLoc(Loc);
      if (FullLoc.isValid()) {
        llvm::outs() << "Found Expr at " << SM.getFilename(Loc) << ":"
                     << FullLoc.getSpellingLineNumber() << ":"
                     << FullLoc.getSpellingColumnNumber() << "\n";
      }
    }

    e->printPretty(llvm::outs(), nullptr,
                   PrintingPolicy(Context->getLangOpts()));
    llvm::outs() << "\n";
    llvm::outs() << "IndirectCount:" << ExprIndirectAccessCount[e] <<"\n";

    // e->dumpColor();
    llvm::outs() << "\n\n";
  }

  std::map<const Expr *, int> ExprIndirectAccessCount;
  std::map<const VarDecl *, int> VarIndirectAccessCount;
  ASTContext *Context;
  SourceManager &SM;
};

class FindIndirectAccessConsumer : public ASTConsumer {
public:
  explicit FindIndirectAccessConsumer(ASTContext *Context) : Visitor(Context) {}

  virtual void HandleTranslationUnit(ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

  FindIndirectAccessVisitor getVisitor() { return Visitor; }

private:
  FindIndirectAccessVisitor Visitor;
};

class FindIndirectAccessAction : public ASTFrontendAction {
public:
  virtual std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::make_unique<FindIndirectAccessConsumer>(
        &Compiler.getASTContext());
  }
};

int main(int argc, const char **argv) {
  llvm::cl::OptionCategory MyToolCategory("my-tool options");
  tooling::CommonOptionsParser op(argc, argv, MyToolCategory);
  tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  Tool.run(tooling::newFrontendActionFactory<FindIndirectAccessAction>().get());
}