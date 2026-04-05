//===- ECSLDialect.td - ECSL dialect definition ------------*- tablegen -*-===//
//
// Defines the `ecsl` MLIR dialect. This is the dialect shell only; no
// operations are declared here yet.
//
//===----------------------------------------------------------------------===//

#ifndef ECSL_IR_ECSL_DIALECT_TD
#define ECSL_IR_ECSL_DIALECT_TD

include "mlir/IR/OpBase.td"

def ECSL_Dialect : Dialect {
  let name = "ecsl";
  let summary = "ECSL dialect for Arcanum";
  let description = [{
    The ECSL dialect carries contract annotations (requires, ensures)
    for functions translated from ECSL-annotated C/C++ sources.
  }];
  let cppNamespace = "::ecsl";
}

#endif // ECSL_IR_ECSL_DIALECT_TD
