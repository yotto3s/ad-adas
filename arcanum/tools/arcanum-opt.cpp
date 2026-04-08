// arcanum-opt: mlir-opt-equivalent driver for Arcanum.
//
// Registers the Arcanum dialects (ecsl, testgen) alongside the
// upstream arith and scf dialects and delegates to MLIR's shared
// `MlirOptMain` implementation, so the binary supports the same
// --parse-only / --mlir-print-ir-* options as mlir-opt.

#include "ecsl/IR/ECSLDialect.h"
#include "testgen/IR/TestGenDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char** argv) {
  mlir::DialectRegistry registry;
  registry.insert<mlir::arith::ArithDialect, mlir::scf::SCFDialect,
                  ecsl::ECSLDialect, testgen::TestGenDialect>();
  return mlir::asMainReturnCode(mlir::MlirOptMain(
      argc, argv, "Arcanum MLIR optimizer driver\n", registry));
}
