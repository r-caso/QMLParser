/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "parser.hpp"

#include <format>

namespace iif_sadaf::talk::QMLParser {

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

std::expected<QMLExpression::Expression, std::string> Parser::sentence()
{
    const auto result = m_EntryPoint(*this);

    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    
    if (peek() != TokenType::EOI) {
        return std::unexpected(std::format("Unexpected symbol: {}", m_TokenList.at(m_Index).literal));
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
            return std::unexpected(result.error());
        }
        QMLExpression::Expression rhs = result.value();
        if (const auto op = m_MapToOperator(TokenType::EQ)) {
            return std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
        }
        return std::unexpected("Unrecognized binary operator");
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
            return std::unexpected(result.error());
        }
        QMLExpression::Expression rhs = result.value();
        if (const auto op = m_MapToOperator(TokenType::IF)) {
            return std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
        }
        return std::unexpected("Unrecognized binary operator.");
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
        const auto op = m_TokenList.at(m_Index).type == TokenType::OR ? m_MapToOperator(TokenType::OR) : m_MapToOperator(TokenType::AND);

        if (!op.has_value()) {
            return std::unexpected("Unrecognized binary operator: " + m_TokenList.at(m_Index).literal);
        }

        advance(); // consume operator

        const auto result = clause();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }

        QMLExpression::Expression rhs = result.value();
        return std::make_shared<QMLExpression::BinaryNode>(*op, lhs, rhs);
    }

    return lhs;
}

std::expected<QMLExpression::Expression, std::string> Parser::clause()
{
    /**************
     * TRY ATOMIC *
     **************/

    std::string error_message;

    if (const auto result = atomic(); result.has_value()) {
        return result.value();
    }
    else {
        if (!result.error().empty()) {
            return std::unexpected(result.error());
        }
        else {
            error_message += std::format("Expected '(', '[', '=' or '≠' after identifier '{}', but got '{}'", m_TokenList.at(m_Index).literal, m_TokenList.at(m_Index + 1).literal);
        }
    }

    /*************
     * TRY UNARY *
     *************/

    if (const auto result = unary(); result.has_value()) {
        return result.value();
    }
    else {
        if (!result.error().empty()) {
            return std::unexpected(result.error());
        }
    }

    /************************
     * TRY QUANTIFICATIONAL *
     ************************/

    if (const auto result = quantificational(); result.has_value()) {
        return result.value();
    }
    else {
        if (!result.error().empty()) {
            return std::unexpected(result.error());
        }
        }

    /*******************
     * TRY ENTRY_POINT *
     *******************/

    if (peek() == TokenType::LPAREN) {
        advance();
        const auto result = m_EntryPoint(*this);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        if (peek() != TokenType::RPAREN) {
            return std::unexpected(std::format("Expected ')' but got '{}'", m_TokenList.at(m_Index).literal));
        }
        advance();
        return result.value();
    }

    if (peek() == TokenType::LBRACKET) {
        advance();
        const auto result = m_EntryPoint(*this);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        if (peek() != TokenType::RBRACKET) {
            return std::unexpected(std::format("Expected ']' but got '{}'", m_TokenList.at(m_Index).literal));
        }
        advance();
        return result.value();
    }
        
    return std::unexpected(error_message);
    }

std::expected<QMLExpression::Expression, std::string> Parser::quantificational()
{
    if (peek() != TokenType::FORALL && peek() != TokenType::EXISTS && peek() != TokenType::NOT_EXISTS) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::VARIABLE) {
        return std::unexpected(std::format("Expected variable after quantifier but got '{}'", m_TokenList.at(m_Index + 1).literal));
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
        return std::unexpected("Non-existent map for token type NOT");
        }
        
        return quantified;
    }

std::expected<QMLExpression::Expression, std::string> Parser::unary()
{
    if (peek() != TokenType::NOT && peek() != TokenType::POS && peek() != TokenType::NEC) {
        return std::unexpected("");
        }

    const auto op = m_TokenList.at(m_Index).type == TokenType::NOT ? m_MapToOperator(TokenType::NOT) :
        m_TokenList.at(m_Index).type == TokenType::POS ? m_MapToOperator(TokenType::POS) :
        m_MapToOperator(TokenType::NEC);

    if (!op.has_value()) {
        return std::unexpected(std::format("Non-existent map for unary operator {}", m_TokenList.at(m_Index).literal));
        }

    advance(); // consume operator

    const auto result = clause();
        if (!result.has_value()) {
        return std::unexpected("Expected clause after unary operator");
        }

    return std::make_shared<QMLExpression::UnaryNode>(*op, result.value());
}

std::expected<QMLExpression::Expression, std::string> Parser::atomic()
{
    std::string error_message;

    if (const auto result = predication(); result.has_value()) {
        return result.value();
    }
    else {
        error_message += result.error();
    }

    if (const auto result = identity(); result.has_value()) {
        return result.value();
    }
    else {
        error_message += result.error();
    }

    if (const auto result = inequality(); result.has_value()) {
        return result.value();
    }
    else {
        error_message += result.error();
    }

    return std::unexpected(error_message);
}

std::expected<QMLExpression::Expression, std::string> Parser::predication()
{
    if (peek() != TokenType::IDENTIFIER) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::LPAREN) {
        return std::unexpected("");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected(std::format("Expected term after '(' but got '{}'", m_TokenList.at(m_Index + 2).literal));
    }

    if (peek(3) != TokenType::RPAREN && peek(3) != TokenType::COMMA) {
        return std::unexpected(std::format("Expected ',' or ')' after term '{}' but got '{}'", m_TokenList.at(m_Index + 2).literal, m_TokenList.at(m_Index + 3).literal));
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
            const std::string error_string = std::format("Expected term after ',' but got '{}'", m_TokenList.at(m_Index + 1).literal);
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
        const std::string error_string = std::format("Expected ')' after argument list but got '{}'", m_TokenList.at(m_Index).literal);
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
        return std::unexpected(std::format("Expected singular term in RHS of '=' but got '{}'", m_TokenList.at(m_Index + 2).literal));
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
        return std::unexpected(std::format("Expected singular term in RHS of '≠' but got '{}'", m_TokenList.at(m_Index + 1).literal));
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
    return std::unexpected("Non-existent map for token type NOT\n");
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