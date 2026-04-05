cmake_minimum_required(VERSION 3.25)
project(Arcanum
  VERSION 0.1.0
  LANGUAGES CXX C
)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Build settings ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# --- Find MLIR (which transitively finds LLVM) ---
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(MLIR REQUIRED)

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "MLIR include dirs: ${MLIR_INCLUDE_DIRS}")

# --- Include LLVM/MLIR CMake modules ---
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

# --- Include paths ---
# LLVM/MLIR treated as SYSTEM includes so their headers don't fire
# Arcanum's stricter warnings (see cmake/Warnings.cmake).
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# --- Compiler warnings ---
include(cmake/Warnings.cmake)

# --- LLVM definitions ---
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# --- TableGen setup ---
# MLIRConfig.cmake sets MLIR_TABLEGEN_EXE to bare "mlir-tblgen".
# Resolve to full path for custom commands to work correctly.
if(NOT IS_ABSOLUTE "${MLIR_TABLEGEN_EXE}")
  find_program(MLIR_TABLEGEN_EXE_FULL "mlir-tblgen"
    HINTS
      "${MLIR_INSTALL_PREFIX}/bin"
      "${LLVM_TOOLS_BINARY_DIR}"
      "${LLVM_INSTALL_PREFIX}/bin"
    REQUIRED
  )
  set(MLIR_TABLEGEN_EXE "${MLIR_TABLEGEN_EXE_FULL}")
endif()
set(MLIR_TABLEGEN_TARGET mlir-tblgen)
message(STATUS "MLIR TableGen executable: ${MLIR_TABLEGEN_EXE}")

# Out-of-tree projects need these dummy targets that AddMLIR.cmake references
if(NOT TARGET mlir-headers)
  add_custom_target(mlir-headers)
endif()
if(NOT TARGET mlir-generic-headers)
  add_custom_target(mlir-generic-headers)
endif()

# --- Testing ---
include(CTest)

# --- Documentation (optional) ---
include(cmake/Doxygen.cmake)

# --- Subdirectories ---
add_subdirectory(ecsl)
add_subdirectory(parser)
add_subdirectory(testgen)
add_subdirectory(verify)
add_subdirectory(tools)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
