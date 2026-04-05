//===- ECSLDialect.cpp - ECSL dialect implementation ----------------------===//
//
// Implements the ECSL MLIR dialect. No operations are registered yet.
//
//===----------------------------------------------------------------------===//

#include "ecsl/IR/ECSLDialect.h"

#include "ecsl/IR/ECSLOps.h" // IWYU pragma: keep (used via GET_OP_LIST)

using namespace ecsl;

#include "ecsl/IR/ECSLOpsDialect.cpp.inc"

void ECSLDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "ecsl/IR/ECSLOps.cpp.inc"
      >();
}
