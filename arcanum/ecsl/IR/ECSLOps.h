/// \file
/// Declares the ECSL dialect operation classes, enums, and attributes.

#ifndef ECSL_IR_ECSLOPS_H
#define ECSL_IR_ECSLOPS_H

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

// clang-format off
#include "ecsl/IR/ECSLEnums.h.inc"
// clang-format on

#define GET_OP_CLASSES
#include "ecsl/IR/ECSLOps.h.inc"

#endif // ECSL_IR_ECSLOPS_H
