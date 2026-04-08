// Smoke test: loads the TestGen dialect into an MLIRContext and
// verifies its registered namespace.

#include "testgen/IR/TestGenDialect.h"

#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/raw_ostream.h"

int main() {
  mlir::MLIRContext ctx;
  ctx.loadDialect<testgen::TestGenDialect>();
  const mlir::Dialect* dialect = ctx.getOrLoadDialect("testgen");
  if (dialect == nullptr) {
    llvm::errs() << "failed to load testgen dialect\n";
    return 1;
  }
  if (dialect->getNamespace() != "testgen") {
    llvm::errs() << "unexpected dialect namespace: " << dialect->getNamespace()
                 << "\n";
    return 1;
  }
  return 0;
}
