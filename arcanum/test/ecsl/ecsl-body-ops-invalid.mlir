// RUN: %arcanum-opt --verify-diagnostics --split-input-file %s

// Negative tests for ecsl body operations: verify that type
// mismatches and invalid symbol references are rejected.

// -----

// Verify ecsl.var rejects element-type mismatch.
module {
  ecsl.func @var_mismatch : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %init = arith.constant 10 : i32
      // expected-error @+1 {{!ecsl.var element type must match init operand type}}
      %v = ecsl.var %init : i32 -> !ecsl.var<i64>
      ecsl.return
    }
}

// -----

// Verify ecsl.store rejects element-type mismatch.
module {
  ecsl.func @store_mismatch : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %init = arith.constant 10 : i32
      %v = ecsl.var %init : i32 -> !ecsl.var<i32>
      %bad = arith.constant 1 : i64
      // expected-error @+1 {{!ecsl.var element type must match stored value type}}
      ecsl.store %v, %bad : !ecsl.var<i32>, i64
      ecsl.return
    }
}

// -----

// Verify ecsl.load rejects element-type mismatch.
module {
  ecsl.func @load_mismatch : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %init = arith.constant 10 : i32
      %v = ecsl.var %init : i32 -> !ecsl.var<i32>
      // expected-error @+1 {{!ecsl.var element type must match load result type}}
      %bad = ecsl.load %v : !ecsl.var<i32> -> i64
      ecsl.return
    }
}

// -----

// Verify ecsl.call rejects references to undefined symbols.
module {
  ecsl.func @call_undefined : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      // expected-error @+1 {{'nonexistent' does not reference a valid ecsl.func}}
      ecsl.call @nonexistent() : () -> ()
      ecsl.return
    }
}

// -----

// Verify ecsl.call rejects operand type mismatch.
module {
  ecsl.func @target : (i32) -> i32
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %z = arith.constant 0 : i32
      ecsl.return %z : i32
    }

  ecsl.func @call_operand_mismatch : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %bad = arith.constant 1 : i64
      // expected-error @+1 {{operand types do not match callee input types}}
      %r = ecsl.call @target(%bad) : (i64) -> i32
      ecsl.return
    }
}

// -----

// Verify ecsl.call rejects result type mismatch.
module {
  ecsl.func @target2 : (i32) -> i32
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %z = arith.constant 0 : i32
      ecsl.return %z : i32
    }

  ecsl.func @call_result_mismatch : () -> ()
    requires { ecsl.clause_yield }
    ensures { ecsl.clause_yield }
    body {
      %arg = arith.constant 1 : i32
      // expected-error @+1 {{result types do not match callee result types}}
      %r = ecsl.call @target2(%arg) : (i32) -> i64
      ecsl.return
    }
}
