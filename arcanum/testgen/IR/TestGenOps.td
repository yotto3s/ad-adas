//===- TestGenOps.td - TestGen dialect operations ---------*- tablegen -*-===//
//
// TableGen source for TestGen dialect operations. No operations are
// defined yet; this file exists so the TableGen to C++ pipeline can
// be exercised.
//
//===----------------------------------------------------------------------===//

#ifndef TESTGEN_IR_TESTGEN_OPS_TD
#define TESTGEN_IR_TESTGEN_OPS_TD

include "testgen/IR/TestGenDialect.td"
include "mlir/IR/OpBase.td"

class TestGen_Op<string mnemonic, list<Trait> traits = []>
    : Op<TestGen_Dialect, mnemonic, traits>;

#endif // TESTGEN_IR_TESTGEN_OPS_TD
