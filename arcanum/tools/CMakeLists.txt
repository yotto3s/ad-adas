# tools/ - CLI Frontend Executables
#
# Targets:
#   arcanum-opt     - mlir-opt-equivalent driver for Arcanum dialects

add_llvm_executable(arcanum-opt arcanum-opt.cpp)
llvm_update_compile_flags(arcanum-opt)

target_link_libraries(arcanum-opt
  PRIVATE
  MLIRECSL
  MLIRTestGen
  MLIRArithDialect
  MLIRSCFDialect
  MLIROptLib
  MLIRIR
  MLIRParser
  MLIRSupport
)

mlir_check_all_link_libraries(arcanum-opt)
