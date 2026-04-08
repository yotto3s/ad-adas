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

  # -Wswitch-default demands a `default:` label on every switch, even
  # ones that already cover every enumerator; that contradicts the
  # more useful -Wcovered-switch-default, which flags unreachable
  # default labels. Keep the latter, drop the former.
  -Wno-switch-default

  # -Wpadded fires on any struct the compiler pads for alignment,
  # which is essentially every non-trivial struct on x86-64. LLVM and
  # most modern C++ codebases disable it.
  -Wno-padded
)

add_compile_options(${ARCANUM_WARNING_FLAGS})

if(ARCANUM_WERROR)
  add_compile_options(-Werror)
  message(STATUS "Arcanum: -Werror enabled (set -DARCANUM_WERROR=OFF to disable)")
else()
  message(STATUS "Arcanum: -Werror disabled")
endif()
