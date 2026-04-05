//===- TestGenDialect.td - TestGen dialect definition -----*- tablegen -*-===//
//
// Defines the `testgen` MLIR dialect. This is the dialect shell only;
// no operations are declared here yet.
//
//===----------------------------------------------------------------------===//

#ifndef TESTGEN_IR_TESTGEN_DIALECT_TD
#define TESTGEN_IR_TESTGEN_DIALECT_TD

include "mlir/IR/OpBase.td"

def TestGen_Dialect : Dialect {
  let name = "testgen";
  let summary = "TestGen dialect for Arcanum";
  let description = [{
    The TestGen dialect represents test-case generation artifacts in
    MLIR. It currently contains no operations; it exists to validate
    the dialect registration pipeline.
  }];
  let cppNamespace = "::testgen";
}

#endif // TESTGEN_IR_TESTGEN_DIALECT_TD
