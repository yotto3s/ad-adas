#include "ecsl/IR/ECSLDialect.h"

#include "ecsl/IR/ECSLOps.h" // IWYU pragma: keep (used via GET_OP_LIST)

using namespace ecsl;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#include "ecsl/IR/ECSLEnums.cpp.inc"
#include "ecsl/IR/ECSLOpsDialect.cpp.inc"

#pragma clang diagnostic pop

void ECSLDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "ecsl/IR/ECSLOps.cpp.inc"
      >();
}
