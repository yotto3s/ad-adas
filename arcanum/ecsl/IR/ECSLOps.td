//===- ECSLOps.td - ECSL dialect operations ---------------*- tablegen -*-===//
//
// Contract operations for the ECSL dialect: `ecsl.func` with
// requires/ensures/body regions, structured constraint operations,
// and `ecsl.return`.
//
//===----------------------------------------------------------------------===//

#ifndef ECSL_IR_ECSL_OPS_TD
#define ECSL_IR_ECSL_OPS_TD

include "ecsl/IR/ECSLDialect.td"
include "mlir/IR/BuiltinAttributes.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/EnumAttr.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class ECSL_Op<string mnemonic, list<Trait> traits = []>
    : Op<ECSL_Dialect, mnemonic, traits>;

//===----------------------------------------------------------------------===//
// Enums
//===----------------------------------------------------------------------===//

def ECSL_CmpPredicate : I32EnumAttr<"CmpPredicate",
    "ECSL comparison predicate", [
  I32EnumAttrCase<"eq", 0>,
  I32EnumAttrCase<"ne", 1>,
  I32EnumAttrCase<"lt", 2>,
  I32EnumAttrCase<"le", 3>,
  I32EnumAttrCase<"gt", 4>,
  I32EnumAttrCase<"ge", 5>,
]> {
  let cppNamespace = "::ecsl";
}

def ECSL_Signedness : I32EnumAttr<"Signedness",
    "ECSL integer signedness", [
  I32EnumAttrCase<"Signed", 0, "signed">,
  I32EnumAttrCase<"Unsigned", 1, "unsigned">,
]> {
  let cppNamespace = "::ecsl";
}

//===----------------------------------------------------------------------===//
// ecsl.func
//===----------------------------------------------------------------------===//

def ECSL_FuncOp : ECSL_Op<"func",
    [Symbol, IsolatedFromAbove]> {
  let summary = "ECSL contract function definition";
  let description = [{
    An `ecsl.func` op defines a function with three regions that carry
    the ECSL contract: `requires` (preconditions), `ensures`
    (postconditions), and `body` (implementation). Multiple clauses in
    `requires` / `ensures` are implicitly conjoined. The `ensures`
    region sees the returned value as a block argument.
  }];

  let arguments = (ins
    SymbolNameAttr:$sym_name,
    TypeAttrOf<FunctionType>:$function_type
  );
  let regions = (region
    AnyRegion:$requires_region,
    AnyRegion:$ensures_region,
    AnyRegion:$body
  );

  let assemblyFormat = [{
    $sym_name `:` $function_type attr-dict-with-keyword
      `requires` $requires_region
      `ensures` $ensures_region
      `body` $body
  }];
}

//===----------------------------------------------------------------------===//
// ecsl.return
//===----------------------------------------------------------------------===//

def ECSL_ReturnOp : ECSL_Op<"return",
    [Terminator, ParentOneOf<["::ecsl::FuncOp"]>]> {
  let summary = "Return from an ecsl.func body";
  let description = [{
    Terminator of an `ecsl.func` body region. The operand value (if
    any) becomes the block argument of the parent function's `ensures`
    region for postcondition checking.
  }];

  let arguments = (ins Variadic<AnyType>:$operands);
  let assemblyFormat = "($operands^ `:` type($operands))? attr-dict";
  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// ecsl.clause_yield
//===----------------------------------------------------------------------===//

def ECSL_ClauseYieldOp : ECSL_Op<"clause_yield",
    [Terminator, ParentOneOf<["::ecsl::FuncOp"]>]> {
  let summary =
    "Terminator yielding clauses from a requires/ensures region";
  let description = [{
    Terminates a `requires` or `ensures` region of an `ecsl.func`.
    Each operand represents one contract clause and must be produced
    by an `ecsl.constraint.*` op. Operands are implicitly conjoined.
  }];

  let arguments = (ins Variadic<I1>:$clauses);
  let assemblyFormat = "($clauses^ `:` type($clauses))? attr-dict";
  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// ecsl.constraint.cmp
//===----------------------------------------------------------------------===//

def ECSL_ConstraintCmpOp : ECSL_Op<"constraint.cmp",
    [Pure, SameTypeOperands]> {
  let summary = "Comparison constraint";
  let description = [{
    Compares two integer operands of the same type using
    `$predicate`, interpreting them according to `$signedness`.
    Produces an `i1` truth value.
  }];

  let arguments = (ins
    ECSL_CmpPredicate:$predicate,
    ECSL_Signedness:$signedness,
    AnyInteger:$lhs,
    AnyInteger:$rhs
  );
  let results = (outs I1:$result);
  let assemblyFormat = [{
    $predicate `,` $signedness `,` $lhs `,` $rhs
      `:` type($lhs) attr-dict
  }];
}

//===----------------------------------------------------------------------===//
// ecsl.constraint.not
//===----------------------------------------------------------------------===//

def ECSL_ConstraintNotOp : ECSL_Op<"constraint.not", [Pure]> {
  let summary = "Logical negation of a constraint";
  let arguments = (ins I1:$operand);
  let results = (outs I1:$result);
  let assemblyFormat = "$operand attr-dict";
}

//===----------------------------------------------------------------------===//
// ecsl.constraint.and / ecsl.constraint.or
//===----------------------------------------------------------------------===//

class ECSL_ConstraintNaryOp<string mnemonic>
    : ECSL_Op<mnemonic, [SingleBlock, NoTerminator]> {
  let arguments = (ins);
  let regions = (region SizedRegion<1>:$body);
  let results = (outs I1:$result);
  let assemblyFormat = "$body attr-dict";
}

def ECSL_ConstraintAndOp : ECSL_ConstraintNaryOp<"constraint.and"> {
  let summary = "Logical conjunction of the constraints in its region";
}

def ECSL_ConstraintOrOp : ECSL_ConstraintNaryOp<"constraint.or"> {
  let summary = "Logical disjunction of the constraints in its region";
}

#endif // ECSL_IR_ECSL_OPS_TD
