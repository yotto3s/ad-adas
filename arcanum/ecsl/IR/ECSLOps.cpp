#include "ecsl/IR/ECSLOps.h"

#include "ecsl/IR/ECSLDialect.h" // IWYU pragma: keep (used by generated .inc)

#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LLVM.h"

#include "llvm/ADT/STLExtras.h"

#include <array>

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
    for (const ::mlir::Value result : innerOp.getResults()) {
      if (result.getType().isInteger(1) && !isConstraintOp(&innerOp)) {
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
  const ::mlir::FunctionType fnType = getFunctionType();
  const ::mlir::TypeRange expectedResults = fnType.getResults();
  for (::mlir::Block &block : getBody()) {
    auto retOp = ::mlir::dyn_cast<ReturnOp>(block.getTerminator());
    if (!retOp) {
      continue;
    }
    const ::mlir::TypeRange actualResults = retOp.getOperands().getTypes();
    if (actualResults.size() != expectedResults.size() ||
        !::llvm::equal(actualResults, expectedResults)) {
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
  for (const ::mlir::Value operand : getClauses()) {
    if (!isConstraintOp(operand.getDefiningOp())) {
      return emitOpError(
          "clause operand must be produced by an ecsl.constraint.* op");
    }
  }
  return ::mlir::success();
}

::mlir::LogicalResult ConstraintCmpOp::verify() {
  static constexpr std::array<unsigned, 5> AllowedWidths{1, 8, 16, 32, 64};
  const auto intType = ::mlir::cast<::mlir::IntegerType>(getLhs().getType());
  const unsigned width = intType.getWidth();
  if (::llvm::find(AllowedWidths, width) == AllowedWidths.end()) {
    return emitOpError("operand width must be one of {1, 8, 16, 32, 64}, "
                       "got i")
           << width;
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
