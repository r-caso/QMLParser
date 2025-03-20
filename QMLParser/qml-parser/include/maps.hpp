#pragma once

#include <optional>

#include "expression.hpp"
#include "lexer.hpp"

namespace iif_sadaf::talk::QMLParser {

std::optional<QMLExpression::Operator> mapToAlethicOperator(TokenType type);
std::optional<QMLExpression::Operator> mapToDeonticOperator(TokenType type);
std::optional<QMLExpression::Operator> mapToEpistemicOperator(TokenType type);

}
