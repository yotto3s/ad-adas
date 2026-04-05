# test/tools/ - CLI tool smoke tests

add_test(NAME arcanum-opt-roundtrip
  COMMAND arcanum-opt
    "${CMAKE_CURRENT_SOURCE_DIR}/arith-scf-roundtrip.mlir"
)
