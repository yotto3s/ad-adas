# test/testgen/ - TestGen dialect unit tests

add_executable(arcanum-test-testgen-register
  RegisterDialect.cpp
)

target_link_libraries(arcanum-test-testgen-register
  PRIVATE
  MLIRTestGen
)

add_test(NAME testgen-register
  COMMAND arcanum-test-testgen-register
)
