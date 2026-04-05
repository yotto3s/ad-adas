/// \file
/// Declares the ECSL dialect operation classes.

#ifndef ECSL_IR_ECSLOPS_H
#define ECSL_IR_ECSLOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"

#define GET_OP_CLASSES
#include "ecsl/IR/ECSLOps.h.inc"

#endif // ECSL_IR_ECSLOPS_H
