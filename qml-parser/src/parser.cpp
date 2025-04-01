/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "parser.hpp"

#include <format>

namespace iif_sadaf::talk::QMLParser {

namespace {
    bool isTerm(TokenType type)
    {
        return type == TokenType::VARIABLE || type == TokenType::IDENTIFIER;
    }

    bool isUnaryOperator(TokenType type)
    {
        return type == TokenType::NOT || type == TokenType::NEC || type == TokenType::POS;
    }

    bool isQuantifier(TokenType type)
    {
        return type == TokenType::FORALL || type == TokenType::EXISTS || type == TokenType::NOT_EXISTS;
    }
}

/**
 * @brief Constructs a Parser instance.
 * @param tokens The list of tokens to parse.
 * @param mapFunc Function that maps tokens to modal operators (default: alethic logic).
 */
Parser::Parser(const std::vector<Token>& tokens, MappingFunction mapFunc)
    : m_Index(0), m_LookAhead(TokenType::NIL), m_TokenList(tokens), m_MapToOperator(mapFunc)
{
    m_LookAhead = m_TokenList.empty() ? TokenType::EOI : m_TokenList.at(0).type;
}

/**
 * @brief Parses the token stream into a QML expression.
 * @param entryPoint The starting parse rule (default: equivalence).
 * @return Parsed QML expression or an error message.
 */
std::expected<QMLExpression::Expression, std::string> Parser::parse(ParseFunction entryPoint)
{
    m_Index = 0;
    m_LookAhead = m_TokenList.empty() ? TokenType::EOI : m_TokenList.at(0).type;

    m_EntryPoint = entryPoint;

    if (m_TokenList.empty()) {
        return std::unexpected("Empty input string, nothing to do");
    }

    return sentence();
}


void Parser::advance()
{
    if (m_LookAhead != TokenType::EOI) {
        m_Index++;
        m_LookAhead = m_Index < m_TokenList.size() ? m_TokenList.at(m_Index).type : TokenType::EOI;
    }
}

TokenType Parser::peek(int offset) const
{
    if (m_Index + offset < m_TokenList.size()) {
        return m_TokenList.at(m_Index + offset).type;
    }
    return TokenType::EOI;
}

const Token& Parser::getToken(size_t index) const {
    if (index < m_TokenList.size()) {
        return m_TokenList.at(index);
    }
    return m_TokenList.at(m_TokenList.size() - 1);
}

std::expected<QMLExpression::Expression, std::string> Parser::sentence()
{
    const auto result = m_EntryPoint(*this);

    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    
    if (peek() != TokenType::EOI) {
        return std::unexpected(std::format("Unexpected symbol ({})", getToken(m_Index).literal));
    }

    return result.value();
}

std::expected<QMLExpression::Expression, std::string> Parser::equivalence()
{
    const auto result = implication();
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    QMLExpression::Expression lhs = result.value();

    while (peek() == TokenType::EQ) {
        advance(); // consume equivalence
        const auto result = implication();
        if (!result.has_value()) {
            return std::unexpected(std::format("Expected clause after '↔' but got : {}", result.error()));
        }
        QMLExpression::Expression rhs = result.value();
        if (const auto op = m_MapToOperator(TokenType::EQ)) {
            lhs = std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
        }
        else {
            return std::unexpected("Non-existent map for token type EQ (↔)");
    }
    }

    return lhs;
}

std::expected<QMLExpression::Expression, std::string> Parser::implication()
{
    const auto result = conjunction_disjunction();
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    
    QMLExpression::Expression lhs = result.value();

    while (peek() == TokenType::IF) {
        advance(); // consume implication
        const auto result = conjunction_disjunction();
        if (!result.has_value()) {
            return std::unexpected(std::format("Expected clause after '→' but got : {}", result.error()));
        }
        QMLExpression::Expression rhs = result.value();
        if (const auto op = m_MapToOperator(TokenType::IF)) {
            lhs = std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
        }
        else {
            return std::unexpected("Non-existent map for token type IMP (→)");
    }
    }

    return lhs;
}

std::expected<QMLExpression::Expression, std::string> Parser::conjunction_disjunction()
{
    const auto result = clause();
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    
    QMLExpression::Expression lhs = result.value();

    while (peek() == TokenType::OR || peek() == TokenType::AND) {
        // Identify operator
        const auto currentToken = getToken(m_Index);
        const auto op = currentToken.type == TokenType::OR ? m_MapToOperator(TokenType::OR) : m_MapToOperator(TokenType::AND);
        const std::string opName = currentToken.type == TokenType::OR ? "OR (∨)" : "AND (∧)";

        if (!op.has_value()) {
            return std::unexpected(std::format("Non-existent map for token type {}", opName));
        }

        advance(); // consume operator
        const auto result = clause();
        if (!result.has_value()) {
            return std::unexpected(std::format("Expected clause after '{}' but got : {}", currentToken.literal, result.error()));
        }
        QMLExpression::Expression rhs = result.value();
        lhs = std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
    }

    return lhs;
}

std::expected<QMLExpression::Expression, std::string> Parser::clause()
{
    const auto currentToken = getToken(m_Index);

    if (isTerm(currentToken.type)) {
        return atomic();
    }

    if (isUnaryOperator(currentToken.type)) {
        return unary();
        }

    if (isQuantifier(currentToken.type)) {
        return quantificational();
    }

    if (currentToken.type == TokenType::LPAREN) {
        advance();
        const auto result = m_EntryPoint(*this);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        if (peek() != TokenType::RPAREN) {
            return std::unexpected(std::format("Expected ')' after '{}' but got '{}'", currentToken.literal, getToken(m_Index).literal));
        }
        advance();
        return result.value();
    }

    if (currentToken.type == TokenType::LBRACKET) {
        advance();
        const auto result = m_EntryPoint(*this);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        if (peek() != TokenType::RBRACKET) {
            return std::unexpected(std::format("Expected ']' after '{}' but got '{}'", currentToken.literal, getToken(m_Index).literal));
        }
        advance();
        return result.value();
    }

    return std::unexpected(std::format("Unexpected token ({})", currentToken.literal));
}

std::expected<QMLExpression::Expression, std::string> Parser::quantificational()
{
    if (peek() != TokenType::FORALL && peek() != TokenType::EXISTS && peek() != TokenType::NOT_EXISTS) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::VARIABLE) {
        return std::unexpected(std::format("Expected variable after '{}' but got '{}'", getToken(m_Index).literal, getToken(m_Index + 1).literal));
    }

    QMLExpression::Quantifier quantifier = m_TokenList.at(m_Index).type == TokenType::FORALL ? QMLExpression::Quantifier::UNIVERSAL : QMLExpression::Quantifier::EXISTENTIAL;
    const bool negated = m_TokenList.at(m_Index).type == TokenType::NOT_EXISTS;

    advance(); // consume quantifier

    QMLExpression::Term variable(m_TokenList.at(m_Index).literal, QMLExpression::Term::Type::VARIABLE);

    advance(); // consume variable

    const auto result = clause();

    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    QMLExpression::Expression quantified = std::make_shared<QMLExpression::QuantificationNode>(quantifier, variable, result.value());

    if (negated) {
        if (const auto op = m_MapToOperator(TokenType::NOT)) {
            return std::make_shared<QMLExpression::UnaryNode>(*op, quantified);
        }
        return std::unexpected("Non-existent map for token type NOT (¬)");
    }

    return quantified;
}

std::expected<QMLExpression::Expression, std::string> Parser::unary()
{
    if (!isUnaryOperator(peek())) {
        return std::unexpected("");
    }

    const auto currentToken = getToken(m_Index);
    const auto op = currentToken.type == TokenType::NOT ? m_MapToOperator(TokenType::NOT) :
                    currentToken.type == TokenType::POS ? m_MapToOperator(TokenType::POS) :
        m_MapToOperator(TokenType::NEC);
    const std::string opName = currentToken.type == TokenType::NOT ? "NOT (¬)" :
                               currentToken.type == TokenType::POS ? "POS (⋄)" :
                                                                     "NEC (□)";

    if (!op.has_value()) {
        return std::unexpected(std::format("Non-existent map for unary operator {}", opName));
    }

    advance(); // consume operator

    const auto result = clause();
    if (!result.has_value()) {
        return std::unexpected(std::format("Expected clause after unary operator {}", opName));
    }

    return std::make_shared<QMLExpression::UnaryNode>(*op, result.value());
}

std::expected<QMLExpression::Expression, std::string> Parser::atomic()
{
    if (peek() != TokenType::IDENTIFIER && peek() != TokenType::VARIABLE) {
        return std::unexpected("");
    }

    if (peek(1) == TokenType::LPAREN) {
        return predication();
    }

    if (peek(1) == TokenType::ID) {
        return identity();
    }

    if (peek(1) == TokenType::NEQ) {
        return inequality();
    }

    return std::unexpected(std::format("Expected '(', '=', or '≠' after '{}' but got '{}'", getToken(m_Index).literal, getToken(m_Index + 1).literal));
}

std::expected<QMLExpression::Expression, std::string> Parser::predication()
{
    if (peek() != TokenType::IDENTIFIER) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::LPAREN) {
        return std::unexpected("");
    }

    if (!isTerm(peek(2))) {
        return std::unexpected(std::format("Expected term after '(' but got '{}'", getToken(m_Index + 2).literal));
    }

    if (peek(3) != TokenType::RPAREN && peek(3) != TokenType::COMMA) {
        return std::unexpected(std::format("Expected ',' or ')' after term '{}' but got '{}'", getToken(m_Index + 2).literal, getToken(m_Index + 3).literal));
    }

    const int backtracking_point = m_Index;

    std::string predicate = m_TokenList.at(m_Index).literal;
    
    advance(); // consume predicate
    advance(); // consume LPAREN

    std::vector<QMLExpression::Term> arguments;

    QMLExpression::Term::Type type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
    arguments.push_back({ m_TokenList.at(m_Index).literal, type });

    advance(); // consume first argument

    while (peek() == TokenType::COMMA) {
        if (peek(1) != TokenType::IDENTIFIER && peek(1) != TokenType::VARIABLE) {
            const std::string error_string = std::format("Expected term after ',' but got '{}'", getToken(m_Index + 1).literal);
            m_Index = backtracking_point;
            m_LookAhead = m_TokenList.at(m_Index).type;
            return std::unexpected(error_string);
        }

        advance(); // consume comma

        QMLExpression::Term::Type type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
        arguments.push_back({ m_TokenList.at(m_Index).literal, type });

        advance(); // consume argument
    }

    if (peek() != TokenType::RPAREN) {
        const std::string error_string = std::format("Expected ')' after argument list but got '{}'", getToken(m_Index).literal);
        m_Index = backtracking_point;
        m_LookAhead = m_TokenList.at(m_Index).type;
        return std::unexpected(error_string);
    }

    advance(); // consume RPAREN

    return std::make_shared<QMLExpression::PredicationNode>(predicate, arguments);
}

std::expected<QMLExpression::Expression, std::string> Parser::identity()
{
    if (peek() != TokenType::IDENTIFIER && peek() != TokenType::VARIABLE) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::ID) {
        return std::unexpected("");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected(std::format("Expected singular term in RHS of '=' but got '{}'", getToken(m_Index + 2).literal));
    }

    QMLExpression::Term::Type type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
    QMLExpression::Term lhs(m_TokenList.at(m_Index).literal, type);

    advance(); // consume lhs
    advance(); // consume ID

    type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
    QMLExpression::Term rhs(m_TokenList.at(m_Index).literal, type);

    advance(); // consume rhs

    return std::make_shared<QMLExpression::IdentityNode>(lhs, rhs);
}

std::expected<QMLExpression::Expression, std::string> Parser::inequality()
{
    if (peek() != TokenType::IDENTIFIER && peek() != TokenType::VARIABLE) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::NEQ) {
        return std::unexpected("");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected(std::format("Expected singular term in RHS of '≠' but got '{}'", getToken(m_Index + 1).literal));
    }

    QMLExpression::Term::Type type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
    QMLExpression::Term lhs(m_TokenList.at(m_Index).literal, type);

    advance(); // consume lhs
    advance(); // consume NEQ operator

    type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
    QMLExpression::Term rhs(m_TokenList.at(m_Index).literal, type);

    advance(); // consume rhs

    QMLExpression::Expression id = std::make_shared<QMLExpression::IdentityNode>(lhs, rhs);
    if (const auto op = m_MapToOperator(TokenType::NOT)) {
        return std::make_shared<QMLExpression::UnaryNode>(*op, id);
    }
    return std::unexpected("Non-existent map for token type NOT (¬)");
}

std::expected<QMLExpression::Expression, std::string> parse(const std::string& formula, Parser::ParseFunction entryPoint, Parser::MappingFunction mapFunction)
{
    const std::vector<Token> tokens = lex(formula);
    return Parser(tokens, mapFunction).parse(entryPoint);
}

std::expected<QMLExpression::Expression, std::string> parse(const std::string& formula, Parser::MappingFunction mapFunction)
{
    const std::vector<Token> tokens = lex(formula);
    return Parser(tokens, mapFunction).parse();
}

}