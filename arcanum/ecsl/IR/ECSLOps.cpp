#include "ecsl/IR/ECSLOps.h"

#include "ecsl/IR/ECSLDialect.h" // IWYU pragma: keep (used by generated .inc)

#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LLVM.h"

#include "llvm/ADT/STLExtras.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#define GET_OP_CLASSES
#include "ecsl/IR/ECSLOps.cpp.inc"

#pragma clang diagnostic pop

namespace ecsl {

namespace {

/// Returns true if \p op is one of the ecsl.constraint.* ops.
bool isConstraintOp(const ::mlir::Operation *op) {
  return (op != nullptr) && ::mlir::isa<ConstraintCmpOp, ConstraintNotOp,
                                        ConstraintAndOp, ConstraintOrOp>(op);
}

/// Shared verifier for ConstraintAndOp / ConstraintOrOp bodies.
::mlir::LogicalResult verifyConstraintNaryBody(::mlir::Operation *parent,
                                               ::mlir::Block &block) {
  for (::mlir::Operation &innerOp : block) {
    for (const ::mlir::Value Result : innerOp.getResults()) {
      if (Result.getType().isInteger(1) && !isConstraintOp(&innerOp)) {
        return parent->emitOpError(
                   "region must only contain ecsl.constraint.* ops as "
                   "i1-producing ops; got ")
               << innerOp.getName();
      }
    }
  }
  return ::mlir::success();
}

} // namespace

::mlir::LogicalResult FuncOp::verify() {
  const ::mlir::FunctionType FnType = getFunctionType();
  const ::mlir::TypeRange ExpectedResults = FnType.getResults();
  for (::mlir::Block &block : getBody()) {
    auto retOp = ::mlir::dyn_cast<ReturnOp>(block.getTerminator());
    if (!retOp) {
      continue;
    }
    const ::mlir::TypeRange ActualResults = retOp.getOperands().getTypes();
    if (ActualResults.size() != ExpectedResults.size() ||
        !::llvm::equal(ActualResults, ExpectedResults)) {
      return retOp.emitOpError(
          "operand types do not match ecsl.func result types");
    }
  }
  return ::mlir::success();
}

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
    if (!isConstraintOp(Operand.getDefiningOp())) {
      return emitOpError(
          "clause operand must be produced by an ecsl.constraint.* op");
    }
  }
  return ::mlir::success();
}

::mlir::LogicalResult ConstraintNotOp::verify() {
  if (!isConstraintOp(getOperand().getDefiningOp())) {
    return emitOpError("operand must be produced by an ecsl.constraint.* op");
  }
  return ::mlir::success();
}

::mlir::LogicalResult ConstraintAndOp::verify() {
  return verifyConstraintNaryBody(*this, getBody().front());
}

::mlir::LogicalResult ConstraintOrOp::verify() {
  return verifyConstraintNaryBody(*this, getBody().front());
}

} // namespace ecsl
