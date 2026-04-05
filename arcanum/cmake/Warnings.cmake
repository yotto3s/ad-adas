# Warnings.cmake - Arcanum compiler warning policy.
#
# Turn on every warning clang knows about (-Weverything), then disable
# the ones we consciously don't want. Applied globally via
# add_compile_options(); layered on top of the warnings LLVM's
# HandleLLVMOptions already enables (-Wall -Wextra, etc.).
#
# LLVM/MLIR headers are treated as SYSTEM includes in the top-level
# CMakeLists.txt, so third-party headers do not fire these flags.

option(ARCANUM_WERROR "Treat warnings as errors" ON)

set(ARCANUM_WARNING_FLAGS
  -Wall
  -Wextra
  -Weverything

  # --- Exclusions ---
  # We target C++17, so C++98-compat complaints are noise.
  -Wno-c++98-compat
  -Wno-c++98-compat-pedantic

  # MLIR TableGen-generated code uses patterns these warnings flag:
  #   `properties` as both member and ctor param (-Wshadow-field*),
  #   adaptor constructors with unused size params (-Wunused-parameter),
  #   struct padding on mandatory member layouts (-Wpadded).
  -Wno-shadow-field
  -Wno-shadow-field-in-constructor
  -Wno-unused-parameter
  -Wno-padded
  # MLIR-generated enum dispatchers omit default: on covered switches.
  -Wno-switch-default
  # MLIR TableGen uses __name__ internal identifiers in generated code.
  -Wno-reserved-identifier
  # MLIR generated code uses raw pointer arithmetic on internal arrays.
  -Wno-unsafe-buffer-usage
  # MLIR generated code freely converts between int and unsigned.
  -Wno-sign-conversion
  # MLIR-generated property setters reuse `attr` as both parameter and
  # local variable name across scope blocks.
  -Wno-shadow
  # MLIR-generated parsers emit extra trailing semicolons in diagnostics.
  -Wno-extra-semi-stmt
)

add_compile_options(${ARCANUM_WARNING_FLAGS})

if(ARCANUM_WERROR)
  add_compile_options(-Werror)
  message(STATUS "Arcanum: -Werror enabled (set -DARCANUM_WERROR=OFF to disable)")
else()
  message(STATUS "Arcanum: -Werror disabled")
endif()
