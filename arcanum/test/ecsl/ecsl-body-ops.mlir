// Round-trip test for ecsl body operations: ecsl.var, ecsl.store,
// ecsl.load, and ecsl.call.

module {
  ecsl.func @callee : (i32) -> i32
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %zero = arith.constant 0 : i32
      ecsl.return %zero : i32
    }

  ecsl.func @with_vars : (i32) -> i32
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %init = arith.constant 10 : i32
      %v = ecsl.var %init : i32 -> !ecsl.var<i32>
      %cur = ecsl.load %v : !ecsl.var<i32> -> i32
      %two = arith.constant 2 : i32
      %next = arith.addi %cur, %two : i32
      ecsl.store %v, %next : !ecsl.var<i32>, i32
      %arg = arith.constant 1 : i32
      %called = ecsl.call @callee(%arg) : (i32) -> i32
      %final = ecsl.load %v : !ecsl.var<i32> -> i32
      ecsl.return %final : i32
    }
}
