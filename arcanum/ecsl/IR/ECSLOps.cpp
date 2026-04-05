#include "ecsl/IR/ECSLOps.h"

#include "ecsl/IR/ECSLDialect.h" // IWYU pragma: keep (used by generated .inc)

#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LLVM.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#define GET_OP_CLASSES
#include "ecsl/IR/ECSLOps.cpp.inc"

#pragma clang diagnostic pop

namespace ecsl {

::mlir::LogicalResult ReturnOp::verify() {
  auto funcOp = (*this)->getParentOfType<FuncOp>();
  const ::mlir::Region *parentRegion = (*this)->getParentRegion();
  if (parentRegion != &funcOp.getBody()) {
    return emitOpError("only allowed in the body region of ecsl.func");
  }
  return ::mlir::success();
}

::mlir::LogicalResult ClauseYieldOp::verify() {
  auto funcOp = (*this)->getParentOfType<FuncOp>();
  const ::mlir::Region *parentRegion = (*this)->getParentRegion();
  if (parentRegion == &funcOp.getBody()) {
    return emitOpError(
        "not allowed in the body region of ecsl.func; use ecsl.return");
  }
  for (const ::mlir::Value Operand : getClauses()) {
    const ::mlir::Operation *def = Operand.getDefiningOp();
    if ((def == nullptr) ||
        !::mlir::isa<ConstraintCmpOp, ConstraintNotOp, ConstraintAndOp,
                     ConstraintOrOp>(def)) {
      return emitOpError(
          "clause operand must be produced by an ecsl.constraint.* op");
    }
  }
  return ::mlir::success();
}

} // namespace ecsl
