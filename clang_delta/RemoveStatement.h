//===----------------------------------------------------------------------===//
//
// Copyright (c) 2012, 2018 The University of Utah
// Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License.  See the file COPYING for details.
//
//===----------------------------------------------------------------------===//

#ifndef REMOVE_UNUSED_MEMBER_VALUE_H
#define REMOVE_UNUSED_MEMBER_VALUE_H

#include <string>
#include "llvm/ADT/DenseMap.h"
#include "Transformation.h"

namespace clang {
  class Stmt;
}

class RemoveStatementAnalysisVisitor;

class RemoveStatement : public Transformation {
friend class RemoveStatementAnalysisVisitor;

public:

  RemoveStatement(const char *TransName, const char *Desc)
    : Transformation(TransName, Desc, true),
      AnalysisVisitor(0)
  { }

  ~RemoveStatement();

private:

  virtual void Initialize(clang::ASTContext &context);

  virtual void HandleTranslationUnit(clang::ASTContext &Ctx);

  RemoveStatementAnalysisVisitor *AnalysisVisitor;

  std::vector<clang::Stmt *> TheStmts;

  // Unimplemented
  RemoveStatement();

  RemoveStatement(const RemoveStatement &);

  void operator=(const RemoveStatement &);

  void removeStatement();
};
#endif
