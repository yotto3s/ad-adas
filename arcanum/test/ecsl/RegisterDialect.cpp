// Smoke test: loads the ECSL dialect into an MLIRContext and verifies
// its registered namespace.

#include "ecsl/IR/ECSLDialect.h"

#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/raw_ostream.h"

int main() {
  mlir::MLIRContext ctx;
  ctx.loadDialect<ecsl::ECSLDialect>();
  const mlir::Dialect* dialect = ctx.getOrLoadDialect("ecsl");
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
