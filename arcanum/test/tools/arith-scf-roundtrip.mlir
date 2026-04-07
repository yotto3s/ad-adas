// RUN: %arcanum-opt %s | FileCheck %s

// Round-trip test: exercises arith and scf dialects in a
// builtin.module. arcanum-opt should parse, verify, and re-emit
// this module without errors.

// CHECK-LABEL: module {
module {
  // CHECK: %[[C0:.*]] = arith.constant 0 : index
  // CHECK: %[[C10:.*]] = arith.constant 10 : index
  // CHECK: %[[C1:.*]] = arith.constant 1 : index
  // CHECK: scf.for
  %c0 = arith.constant 0 : index
  %c10 = arith.constant 10 : index
  %c1 = arith.constant 1 : index
  %sum = scf.for %i = %c0 to %c10 step %c1
      iter_args(%acc = %c0) -> (index) {
    // CHECK: arith.addi
    %next = arith.addi %acc, %i : index
    scf.yield %next : index
  }
}
