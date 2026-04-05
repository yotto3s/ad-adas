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

// TableGen-generated code uses patterns that don't align with our
// strict warning policy; silence warnings for the .inc scope only.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

// clang-format off
#include "ecsl/IR/ECSLEnums.h.inc"
// clang-format on

#define GET_OP_CLASSES
#include "ecsl/IR/ECSLOps.h.inc"

#pragma clang diagnostic pop

#endif // ECSL_IR_ECSLOPS_H
