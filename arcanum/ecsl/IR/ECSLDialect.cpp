#include "ecsl/IR/ECSLDialect.h"

#include "ecsl/IR/ECSLOps.h" // IWYU pragma: keep (used via GET_OP_LIST)

#include "llvm/ADT/TypeSwitch.h" // IWYU pragma: keep (used by generated .inc)

using namespace ecsl;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#include "ecsl/IR/ECSLEnums.cpp.inc"
#include "ecsl/IR/ECSLOpsDialect.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "ecsl/IR/ECSLTypes.cpp.inc"

#pragma clang diagnostic pop

void ECSLDialect::initialize() {
  // clang-analyzer mis-reports a stack-address escape through
  // MLIR's AbstractType::get<T> lambda capture.
  // NOLINTBEGIN(clang-analyzer-core.StackAddressEscape)
  addTypes<
#define GET_TYPEDEF_LIST
#include "ecsl/IR/ECSLTypes.cpp.inc"
      >();
  // NOLINTEND(clang-analyzer-core.StackAddressEscape)
  addOperations<
#define GET_OP_LIST
#include "ecsl/IR/ECSLOps.cpp.inc"
      >();
}
