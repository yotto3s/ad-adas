// RUN: %arcanum-opt %s | FileCheck %s

// Round-trip test for ecsl contract operations: ecsl.func with
// requires/ensures/body regions, constraint ops (cmp, not, and, or),
// ecsl.clause_yield as the requires/ensures terminator, and
// ecsl.return.

// CHECK-LABEL: ecsl.func @contract_demo
module {
  ecsl.func @contract_demo : (i32, i32) -> i32
    requires {
      %a = arith.constant 0 : i32
      %b = arith.constant 100 : i32
      // CHECK: ecsl.constraint.cmp ge, signed
      %c1 = ecsl.constraint.cmp ge, signed, %a, %b : i32
      // CHECK: ecsl.constraint.cmp le, signed
      %c2 = ecsl.constraint.cmp le, signed, %a, %b : i32
      // CHECK: ecsl.constraint.not
      %not_c1 = ecsl.constraint.not %c1
      // CHECK: ecsl.constraint.and
      %and_result = ecsl.constraint.and {
        %inner = arith.constant 5 : i32
        %c3 = ecsl.constraint.cmp gt, signed, %b, %inner : i32
        %c4 = ecsl.constraint.cmp lt, signed, %inner, %b : i32
        %c5 = ecsl.constraint.not %c3
      }
      // CHECK: ecsl.constraint.or
      %or_result = ecsl.constraint.or {
        %zero = arith.constant 0 : i32
        %c6 = ecsl.constraint.cmp eq, unsigned, %a, %zero : i32
        %c7 = ecsl.constraint.cmp ne, unsigned, %b, %zero : i32
      }
      // CHECK: ecsl.clause_yield
      ecsl.clause_yield %c1, %c2, %not_c1, %and_result, %or_result
          : i1, i1, i1, i1, i1
    }
    ensures {
      ecsl.clause_yield
    }
    body {
      // CHECK: ecsl.return
      %r = arith.constant 42 : i32
      ecsl.return %r : i32
    }
}
