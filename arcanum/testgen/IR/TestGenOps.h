/// \file
/// Declares the TestGen dialect operation classes.

#ifndef TESTGEN_IR_TESTGENOPS_H
#define TESTGEN_IR_TESTGENOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"

#define GET_OP_CLASSES
#include "testgen/IR/TestGenOps.h.inc"

#endif // TESTGEN_IR_TESTGENOPS_H
