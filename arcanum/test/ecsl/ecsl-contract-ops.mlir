// Round-trip test for ecsl contract operations: ecsl.func with
// requires/ensures/body regions, constraint ops (cmp, not, and, or),
// ecsl.clause_yield as the requires/ensures terminator, and
// ecsl.return.

module {
  ecsl.func @contract_demo : (i32, i32) -> i32
    requires {
      %a = arith.constant 0 : i32
      %b = arith.constant 100 : i32
      %c1 = ecsl.constraint.cmp ge, signed, %a, %b : i32
      %c2 = ecsl.constraint.cmp le, signed, %a, %b : i32
      %not_c1 = ecsl.constraint.not %c1
      %and_result = ecsl.constraint.and {
        %inner = arith.constant 5 : i32
        %cmp = ecsl.constraint.cmp eq, unsigned, %inner, %inner : i32
      }
      %or_result = ecsl.constraint.or {
      }
      ecsl.clause_yield %c1, %c2, %not_c1, %and_result, %or_result
          : i1, i1, i1, i1, i1
    }
    ensures {
      ecsl.clause_yield
    }
    body {
      %r = arith.constant 42 : i32
      ecsl.return %r : i32
    }
}
