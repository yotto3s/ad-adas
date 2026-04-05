# FindMLIR.cmake - Locate MLIR installation
#
# Searches common LLVM installation prefixes for MLIRConfig.cmake,
# then delegates to config-mode discovery.
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
)

if(DEFINED ENV{LLVM_DIR} AND NOT "$ENV{LLVM_DIR}" STREQUAL "")
  list(APPEND _mlir_search_paths "$ENV{LLVM_DIR}/../mlir")
endif()

if(DEFINED ENV{MLIR_DIR} AND NOT "$ENV{MLIR_DIR}" STREQUAL "")
  list(APPEND _mlir_search_paths "$ENV{MLIR_DIR}")
endif()

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
