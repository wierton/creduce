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

#include <cctype>
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

#include "TransformationManager.h"

using namespace clang;

static const char *DescriptionMsg =
"Remove statement. \n";

static RegisterTransformation<RemoveStatement>
         Trans("remove-stmt", DescriptionMsg);

class RemoveStatementAnalysisVisitor : public
  RecursiveASTVisitor<RemoveStatementAnalysisVisitor> {
public:

  explicit RemoveStatementAnalysisVisitor(RemoveStatement *Instance)
    : ConsumerInstance(Instance)
  { }

  bool VisitStmt(Stmt *S);

private:

  RemoveStatement *ConsumerInstance;
};

bool RemoveStatementAnalysisVisitor::VisitStmt(Stmt *S)
{
  if (ConsumerInstance->isInIncludedFile(S)
    || clang::isa<Expr>(S))
    return true;

  ConsumerInstance->ValidInstanceNum++;
  if (ConsumerInstance->ValidInstanceNum ==
      ConsumerInstance->TransformationCounter) {
    ConsumerInstance->TheStmt = S;
  }
  return true;
}

void RemoveStatement::Initialize(ASTContext &context)
{
  Transformation::Initialize(context);
  AnalysisVisitor = new RemoveStatementAnalysisVisitor(this);
}

void RemoveStatement::HandleTranslationUnit(ASTContext &Ctx)
{
  AnalysisVisitor->TraverseDecl(Ctx.getTranslationUnitDecl());

  if (QueryInstanceOnly)
    return;

  if (TransformationCounter > ValidInstanceNum) {
    TransError = TransMaxInstanceError;
    return;
  }

  Ctx.getDiagnostics().setSuppressAllDiagnostics(false);

  TransAssert(TheStmt && "NULL TheStmt!");

  removeStatement();

  if (Ctx.getDiagnostics().hasErrorOccurred() ||
      Ctx.getDiagnostics().hasFatalErrorOccurred())
    TransError = TransInternalError;
}

static int getOffset(const char *Buf, char Symbol)
{
  int Offset = 0;
  while (*Buf != Symbol) {
    Buf--;
    if (*Buf == '\0')
      break;
    Offset--;
  }
  return Offset;
}

void RemoveStatement::removeStatement()
{
  SourceManager &SrcManager = TheRewriter.getSourceMgr();
  SourceRange Range = TheStmt->getSourceRange();
  TheRewriter.RemoveText(Range);
}

RemoveStatement::~RemoveStatement()
{
  delete AnalysisVisitor;
}

