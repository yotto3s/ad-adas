# ecsl/IR/ - ECSL dialect IR (TableGen + C++)

set(LLVM_TARGET_DEFINITIONS ECSLOps.td)
mlir_tablegen(ECSLOpsDialect.h.inc -gen-dialect-decls -dialect=ecsl)
mlir_tablegen(ECSLOpsDialect.cpp.inc -gen-dialect-defs -dialect=ecsl)
mlir_tablegen(ECSLOps.h.inc -gen-op-decls)
mlir_tablegen(ECSLOps.cpp.inc -gen-op-defs)
mlir_tablegen(ECSLEnums.h.inc -gen-enum-decls)
mlir_tablegen(ECSLEnums.cpp.inc -gen-enum-defs)
add_public_tablegen_target(ECSLOpsIncGen)

add_mlir_dialect_library(MLIRECSL
  ECSLDialect.cpp
  ECSLOps.cpp

  DEPENDS
  ECSLOpsIncGen

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRSupport
)
