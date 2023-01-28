//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2013, 2015 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "RemoveStatement.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include <cctype>

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg = "Remove statement. \n";

static RegisterTransformation<RemoveStatement> Trans(
    "remove-stmt", DescriptionMsg);

class RemoveStatementAnalysisVisitor
    : public RecursiveASTVisitor<RemoveStatementAnalysisVisitor> {
  using VisitorTy = RecursiveASTVisitor<RemoveStatementAnalysisVisitor>;

public:
  explicit RemoveStatementAnalysisVisitor(RemoveStatement *Instance)
      : ConsumerInstance(Instance) {}

  bool TraverseFunctionDecl(FunctionDecl *FD);
  bool VisitStmt(Stmt *S);
  bool TraverseDeclStmt(DeclStmt *S) {
    VisitStmt(S);
    return true;
  }

private:
  RemoveStatement *ConsumerInstance;
  FunctionDecl *currentFunction;
};

bool RemoveStatementAnalysisVisitor::TraverseFunctionDecl(FunctionDecl *FD) {
  currentFunction = FD;
  if (FD->getBody()) VisitorTy::TraverseStmt(FD->getBody());
  currentFunction = nullptr;
  return true;
}

bool RemoveStatementAnalysisVisitor::VisitStmt(Stmt *S) {
  if (!currentFunction ||
      ConsumerInstance->isInIncludedFile(S)
      // || clang::isa<DeclStmt>(S)
      || clang::isa<LabelStmt>(S))
    return true;

  // check if is a nested expr
  if (clang::isa<Expr>(S)) {
    const auto &parents = ConsumerInstance->Context->getParents(*S);
    if (!parents.empty() && !parents[0].get<clang::CompoundStmt>()) return true;
  }

  ConsumerInstance->ValidInstanceNum++;
  ConsumerInstance->TheStmts.push_back(S);
  return true;
}

void RemoveStatement::Initialize(ASTContext &context) {
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveStatementAnalysisVisitor(this);
}

void RemoveStatement::HandleTranslationUnit(ASTContext &Ctx) {
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly) return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  removeStatement();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

void RemoveStatement::removeStatement() {
  unsigned N = TheStmts.size();
  for (int I = TransformationCounter; I < ToCounter; ++I) {
    TransAssert((I >= 1 && I <= N) && "Invalid Index!");
    SourceRange Range = TheStmts.at(N - I)->getSourceRange();
    TheRewriter.RemoveText(Range);
  }
}

RemoveStatement::~RemoveStatement() { delete AnalysisVisitor; }
