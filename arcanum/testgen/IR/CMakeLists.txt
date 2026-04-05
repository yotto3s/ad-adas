# testgen/IR/ - TestGen dialect IR (TableGen + C++)

set(LLVM_TARGET_DEFINITIONS TestGenOps.td)
mlir_tablegen(TestGenOpsDialect.h.inc -gen-dialect-decls -dialect=testgen)
mlir_tablegen(TestGenOpsDialect.cpp.inc -gen-dialect-defs -dialect=testgen)
mlir_tablegen(TestGenOps.h.inc -gen-op-decls)
mlir_tablegen(TestGenOps.cpp.inc -gen-op-defs)
add_public_tablegen_target(TestGenOpsIncGen)

add_mlir_dialect_library(MLIRTestGen
  TestGenDialect.cpp
  TestGenOps.cpp

  DEPENDS
  TestGenOpsIncGen

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRSupport
)
