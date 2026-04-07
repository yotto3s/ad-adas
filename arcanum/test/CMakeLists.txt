# test/ - Test Infrastructure

# --- Unit tests (C++ executables, run via CTest) ---
add_subdirectory(ecsl)
add_subdirectory(testgen)
add_subdirectory(tools)

# --- Lit tests (.mlir files, run via llvm-lit) ---
find_program(LIT_COMMAND
  NAMES lit llvm-lit
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  REQUIRED
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY
)

add_test(NAME check-arcanum-lit
  COMMAND ${LIT_COMMAND} ${CMAKE_CURRENT_BINARY_DIR}
    --verbose
)

# Lit tests depend on arcanum-opt being built.
set_tests_properties(check-arcanum-lit PROPERTIES
  DEPENDS arcanum-opt
)
