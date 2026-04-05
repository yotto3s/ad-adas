// Test input for arcanum-opt: exercises arith and scf dialects in a
// builtin.module. arcanum-opt should parse, verify, and re-emit this
// module without errors.

module {
  %c0 = arith.constant 0 : index
  %c10 = arith.constant 10 : index
  %c1 = arith.constant 1 : index
  %sum = scf.for %i = %c0 to %c10 step %c1
      iter_args(%acc = %c0) -> (index) {
    %next = arith.addi %acc, %i : index
    scf.yield %next : index
  }
}
