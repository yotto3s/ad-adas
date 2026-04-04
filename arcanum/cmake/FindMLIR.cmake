# FindMLIR.cmake - Helper to locate MLIR installation
#
# LLVM 22 ships proper CMake config files (MLIRConfig.cmake), so this
# module searches common installation prefixes as a convenience fallback.
# The user can bypass this by setting MLIR_DIR directly.
#
# Sets:
#   MLIR_FOUND - TRUE if MLIRConfig.cmake was found
#   MLIR_DIR   - Path to the directory containing MLIRConfig.cmake

if(MLIR_FOUND)
  return()
endif()

set(_mlir_search_paths
  /usr/lib/llvm-22/lib/cmake/mlir
  /usr/local/lib/cmake/mlir
  /opt/llvm-22/lib/cmake/mlir
  $ENV{LLVM_DIR}/../mlir
  $ENV{MLIR_DIR}
)

find_package(MLIR CONFIG
  HINTS ${_mlir_search_paths}
)

if(MLIR_FOUND)
  message(STATUS "FindMLIR: Found MLIR at ${MLIR_DIR}")
else()
  message(FATAL_ERROR
    "FindMLIR: Could not find MLIRConfig.cmake.\n"
    "Set MLIR_DIR to the directory containing MLIRConfig.cmake, e.g.:\n"
    "  cmake -DMLIR_DIR=/usr/lib/llvm-22/lib/cmake/mlir .."
  )
endif()
