# test/parser/ - Parser unit tests

add_executable(arcanum-test-ecsl-parser
  ECSLParserTest.cpp
)

target_link_libraries(arcanum-test-ecsl-parser
  PRIVATE
  ArcanumParser
)

add_test(NAME ecsl-parser
  COMMAND arcanum-test-ecsl-parser
)
