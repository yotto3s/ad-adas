# parser/ - Frontend Parser
#
# Components:
#   ECSLParser   - ECSL annotation parser
#   ClangBridge  - Clang AST to ecsl MLIR dialect lowering (future)

add_library(ArcanumParser STATIC
  ECSLParser.cpp
)

target_include_directories(ArcanumParser PUBLIC
  ${PROJECT_SOURCE_DIR}
)
