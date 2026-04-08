/// \file
/// Stub implementations for the ECSL contract parser public API.
/// The lexer and parser are added in subsequent commits.

#include "parser/ECSLParser.h"

#include <string>
#include <string_view>

namespace arcanum::parser {

ParseResult parseContract(std::string_view /*input*/) { return {}; }

std::string toString(const Expr& /*expr*/) { return {}; }

std::string toString(const Clause& /*clause*/) { return {}; }

} // namespace arcanum::parser
