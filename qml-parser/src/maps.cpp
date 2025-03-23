#include "maps.hpp"

namespace iif_sadaf::talk::QMLParser {

std::optional<QMLExpression::Operator> mapToAlethicOperator(TokenType type) {
    switch (type) {
        case TokenType::NOT: return QMLExpression::Operator::NEGATION;
        case TokenType::AND: return QMLExpression::Operator::CONJUNCTION;
        case TokenType::OR: return QMLExpression::Operator::DISJUNCTION;
        case TokenType::IF: return QMLExpression::Operator::CONDITIONAL;
        case TokenType::EQ: return QMLExpression::Operator::BICONDITIONAL;
        case TokenType::NEC: return QMLExpression::Operator::NECESSITY;
        case TokenType::POS: return QMLExpression::Operator::POSSIBILITY;
        default: return std::nullopt;
    }
}

std::optional<QMLExpression::Operator> mapToDeonticOperator(TokenType type) {
    switch (type) {
        case TokenType::NOT: return QMLExpression::Operator::NEGATION;
        case TokenType::AND: return QMLExpression::Operator::CONJUNCTION;
        case TokenType::OR: return QMLExpression::Operator::DISJUNCTION;
        case TokenType::IF: return QMLExpression::Operator::CONDITIONAL;
        case TokenType::EQ: return QMLExpression::Operator::BICONDITIONAL;
        case TokenType::NEC: return QMLExpression::Operator::DEONTIC_NECESSITY;
        case TokenType::POS: return QMLExpression::Operator::DEONTIC_POSSIBILITY;
        default: return std::nullopt;
    }
}

std::optional<QMLExpression::Operator> mapToEpistemicOperator(TokenType type) {
    switch (type) {
    case TokenType::NOT: return QMLExpression::Operator::NEGATION;
    case TokenType::AND: return QMLExpression::Operator::CONJUNCTION;
    case TokenType::OR: return QMLExpression::Operator::DISJUNCTION;
    case TokenType::IF: return QMLExpression::Operator::CONDITIONAL;
    case TokenType::EQ: return QMLExpression::Operator::BICONDITIONAL;
    case TokenType::NEC: return QMLExpression::Operator::EPISTEMIC_NECESSITY;
    case TokenType::POS: return QMLExpression::Operator::EPISTEMIC_POSSIBILITY;
    default: return std::nullopt;
    }
}


}