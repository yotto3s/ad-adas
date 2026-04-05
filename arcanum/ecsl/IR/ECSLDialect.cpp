#include "ecsl/IR/ECSLDialect.h"

#include "ecsl/IR/ECSLOps.h" // IWYU pragma: keep (used via GET_OP_LIST)

using namespace ecsl;

#include "ecsl/IR/ECSLEnums.cpp.inc"
#include "ecsl/IR/ECSLOpsDialect.cpp.inc"

void ECSLDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "ecsl/IR/ECSLOps.cpp.inc"
      >();
}
