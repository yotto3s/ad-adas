# Warnings.cmake - Arcanum compiler warning policy.
#
# Applied globally via add_compile_options(). Layered on top of the
# warnings LLVM's HandleLLVMOptions already enables
# (-Wall -Wextra -Wsuggest-override -Wmisleading-indentation, etc.).
#
# LLVM/MLIR headers are treated as SYSTEM includes elsewhere in the
# top-level CMakeLists.txt, so these flags do not fire on third-party
# code.

option(ARCANUM_WERROR "Treat warnings as errors" ON)

set(ARCANUM_WARNING_FLAGS
  -Wshadow
  -Wold-style-cast
  -Wcast-align
  -Woverloaded-virtual
  -Wformat=2
  -Wnull-dereference
  -Wdouble-promotion
)

add_compile_options(${ARCANUM_WARNING_FLAGS})

if(ARCANUM_WERROR)
  add_compile_options(-Werror)
  message(STATUS "Arcanum: -Werror enabled (set -DARCANUM_WERROR=OFF to disable)")
else()
  message(STATUS "Arcanum: -Werror disabled")
endif()
