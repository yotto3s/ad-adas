//===- RegisterDialect.cpp - ECSL dialect registration smoke test ---------===//
//
// Minimal smoke test that verifies the ECSL dialect can be loaded into
// an MLIRContext and retrieved by its registered name.
//
//===----------------------------------------------------------------------===//

#include "ecsl/IR/ECSLDialect.h"

#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/raw_ostream.h"

int main() {
  mlir::MLIRContext ctx;
  ctx.loadDialect<ecsl::ECSLDialect>();
  const mlir::Dialect *dialect = ctx.getOrLoadDialect("ecsl");
  if (dialect == nullptr) {
    llvm::errs() << "failed to load ecsl dialect\n";
    return 1;
  }
  if (dialect->getNamespace() != "ecsl") {
    llvm::errs() << "unexpected dialect namespace: " << dialect->getNamespace()
                 << "\n";
    return 1;
  }
  return 0;
}
