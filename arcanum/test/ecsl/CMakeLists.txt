# test/ecsl/ - ECSL dialect unit tests

add_executable(arcanum-test-ecsl-register
  RegisterDialect.cpp
)

target_link_libraries(arcanum-test-ecsl-register
  PRIVATE
  MLIRECSL
  MLIRIR
  MLIRSupport
)

add_test(NAME ecsl-register
  COMMAND arcanum-test-ecsl-register
)
