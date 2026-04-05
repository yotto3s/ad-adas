//===- ECSLOps.td - ECSL dialect operations ---------------*- tablegen -*-===//
//
// TableGen source for ECSL dialect operations. No operations are
// defined yet; this file exists so the TableGen → C++ pipeline can be
// exercised.
//
//===----------------------------------------------------------------------===//

#ifndef ECSL_IR_ECSL_OPS_TD
#define ECSL_IR_ECSL_OPS_TD

include "ecsl/IR/ECSLDialect.td"
include "mlir/IR/OpBase.td"

class ECSL_Op<string mnemonic, list<Trait> traits = []>
    : Op<ECSL_Dialect, mnemonic, traits>;

#endif // ECSL_IR_ECSL_OPS_TD
