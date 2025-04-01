/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "parser.hpp"

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
        m_LookAhead = m_TokenList.at(m_Index).type;
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
    m_LookAhead = peek();

    const auto result = m_EntryPoint(*this);

    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    
    if (peek() != TokenType::EOI) {
        return std::unexpected("Expected EOI");
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
        return std::unexpected("Unrecognized binary operator.");
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
            return std::unexpected("Unrecognized binary operator.");
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

    if (const auto result = atomic(); result.has_value()) {
        return result.value();
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
            return std::unexpected(std::format("Expected closing parenthesis but got {}", m_TokenList.at(m_Index).literal));
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
        
        return std::make_shared<QMLExpression::UnaryNode>(*op, result.value());
    }

std::expected<QMLExpression::Expression, std::string> Parser::quantificational()
{
    if (peek() != TokenType::FORALL && peek() != TokenType::EXISTS && peek() != TokenType::NOT_EXISTS) {
        return std::unexpected("");
    }

    if (peek(1) != TokenType::VARIABLE) {
        return std::unexpected("Expected variable after quantifier");
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
            return std::unexpected("Unrecognized unary operator.");
        }
        
        return quantified;
    }

std::expected<QMLExpression::Expression, std::string> Parser::unary()
{
    if (peek() != TokenType::NOT && peek() != TokenType::POS && peek() != TokenType::NEC) {
        return std::unexpected("");
        }
        if (peek() != TokenType::RPAREN) {
            return std::unexpected("Expected closing parenthesis");
        }
        advance();
        return result.value();
    }

    advance(); // consume operator

    const auto result = clause();
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        if (peek() != TokenType::RBRACKET) {
            return std::unexpected("Expected closing bracket");
        }
        advance();
        return result.value();
    }

    return std::make_shared<QMLExpression::UnaryNode>(*op, result.value());
}

std::expected<QMLExpression::Expression, std::string> Parser::atomic()
{
    if (const auto result = predication(); result.has_value()) {
        return result.value();
    }

    if (const auto result = identity(); result.has_value()) {
        return result.value();
    }

    if (const auto result = inequality(); result.has_value()) {
        return result.value();
    }

    return std::unexpected("Error when evaluating atomic.");
}

std::expected<QMLExpression::Expression, std::string> Parser::predication()
{
    if (peek() != TokenType::IDENTIFIER) {
        return std::unexpected("Expected predicate");
    }

    if (peek(1) != TokenType::LPAREN) {
        return std::unexpected("Expected open parenthesis");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected("Expected singular term");
    }

    if (peek(3) != TokenType::RPAREN && peek(3) != TokenType::COMMA) {
        return std::unexpected("Expected punctuation after singular term");
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
            m_Index = backtracking_point;
            m_LookAhead = m_TokenList.at(m_Index).type;
            return std::unexpected("Expected singular term");
        }

        advance(); // consume comma

        QMLExpression::Term::Type type = m_TokenList.at(m_Index).type == TokenType::VARIABLE ? QMLExpression::Term::Type::VARIABLE : QMLExpression::Term::Type::CONSTANT;
        arguments.push_back({ m_TokenList.at(m_Index).literal, type });

        advance(); // consume argument
    }

    if (peek() != TokenType::RPAREN) {
        m_Index = backtracking_point;
        m_LookAhead = m_TokenList.at(m_Index).type;
        return std::unexpected("Expected closing parenthesis");
    }

    advance(); // consume RPAREN

    return std::make_shared<QMLExpression::PredicationNode>(predicate, arguments);
}

std::expected<QMLExpression::Expression, std::string> Parser::identity()
{
    if (peek() != TokenType::IDENTIFIER && peek() != TokenType::VARIABLE) {
        return std::unexpected("Expected singular term in LHS");
    }

    if (peek(1) != TokenType::ID) {
        return std::unexpected("Expected identity symbol");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected("Expected singular term in RHS");
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
        return std::unexpected("Expected singular term in LHS");
    }

    if (peek(1) != TokenType::NEQ) {
        return std::unexpected("Expected inequality symbol");
    }

    if (peek(2) != TokenType::IDENTIFIER && peek(2) != TokenType::VARIABLE) {
        return std::unexpected("Expected singular term in RHS");
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
        return std::make_shared<QMLExpression::UnaryNode>(QMLExpression::Operator::NEGATION, id);
    }
    return std::unexpected("Unrecognized unary operator.");
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