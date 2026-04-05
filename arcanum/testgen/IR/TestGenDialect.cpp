#include "testgen/IR/TestGenDialect.h"

#include "testgen/IR/TestGenOps.h" // IWYU pragma: keep (used via GET_OP_LIST)

using namespace testgen;

#include "testgen/IR/TestGenOpsDialect.cpp.inc"

void TestGenDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "testgen/IR/TestGenOps.cpp.inc"
      >();
}
