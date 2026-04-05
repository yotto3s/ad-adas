# Doxygen.cmake - Optional Doxygen documentation target.
#
# If Doxygen is found on the system, defines a `docs` custom target that
# generates HTML documentation under ${CMAKE_BINARY_DIR}/docs/html. When
# Doxygen is absent, emits a status message and skips the target so the
# main build is unaffected.
#
# Usage:
#   cmake --build <build-dir> --target docs

find_package(Doxygen QUIET)

if(NOT DOXYGEN_FOUND)
  message(STATUS
    "Doxygen not found; skipping 'docs' target. "
    "Install doxygen to enable documentation generation.")
  return()
endif()

set(ARCANUM_DOXYGEN_INPUT "${PROJECT_SOURCE_DIR}")
set(ARCANUM_DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")
set(ARCANUM_DOXYFILE_IN "${PROJECT_SOURCE_DIR}/cmake/Doxyfile.in")
set(ARCANUM_DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")

configure_file("${ARCANUM_DOXYFILE_IN}" "${ARCANUM_DOXYFILE_OUT}" @ONLY)

file(MAKE_DIRECTORY "${ARCANUM_DOXYGEN_OUTPUT_DIR}")

add_custom_target(docs
  COMMAND "${DOXYGEN_EXECUTABLE}" "${ARCANUM_DOXYFILE_OUT}"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "Generating Doxygen documentation"
  VERBATIM
)

message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE} "
  "(run 'cmake --build <dir> --target docs')")
