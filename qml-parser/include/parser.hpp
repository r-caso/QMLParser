/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <vector>
#include <string>

#include <QMLExpression/expression.hpp>

#include "lexer.hpp"
#include "maps.hpp"
#include "token.hpp"

namespace iif_sadaf::talk::QMLParser {

/**
 * @class Parser
 * @brief Parses a sequence of tokens into a Quantified Modal Logic (QML) expression tree.
 *
 * This class implements a recursive descent parser for QML expressions. It processes a vector
 * of tokens and constructs an abstract syntax tree (AST) representing a well-formed formula.
 */
class Parser
{
public:
    /**
     * @typedef ParseFunction
     * @brief Defines the function signature for parsing rules.
     */
    using ParseFunction = std::function<std::expected<QMLExpression::Expression, std::string>(Parser&)>;

    /**
     * @typedef MappingFunction
     * @brief Maps token types to QML logical operators.
     */
    using MappingFunction = std::function<std::optional<QMLExpression::Operator>(TokenType)>;

    Parser(const std::vector<Token>& tokens, MappingFunction mapFunc = mapToAlethicOperator);
    std::expected<QMLExpression::Expression, std::string> parse(ParseFunction entryPoint = &Parser::equivalence);

    // rules
    std::expected<QMLExpression::Expression, std::string> equivalence();
    std::expected<QMLExpression::Expression, std::string> implication();
    std::expected<QMLExpression::Expression, std::string> conjunction_disjunction();
    std::expected<QMLExpression::Expression, std::string> clause();
    std::expected<QMLExpression::Expression, std::string> quantificational();
    std::expected<QMLExpression::Expression, std::string> unary();
    std::expected<QMLExpression::Expression, std::string> atomic();
    std::expected<QMLExpression::Expression, std::string> predication();
    std::expected<QMLExpression::Expression, std::string> identity();
    std::expected<QMLExpression::Expression, std::string> inequality();

private:
    void advance();
    TokenType peek(int offset = 0) const;
    const Token& getToken(size_t index) const;

    // start rule
    std::expected<QMLExpression::Expression, std::string> sentence();

    int m_Index;
    TokenType m_LookAhead;
    std::vector<Token> m_TokenList;
    std::function<std::optional<QMLExpression::Operator>(TokenType)> m_MapToOperator;
    ParseFunction m_EntryPoint = &Parser::equivalence;
};

std::expected<QMLExpression::Expression, std::string> parse(const std::string& formula, Parser::ParseFunction entryPoint = &Parser::equivalence, Parser::MappingFunction mappingFunction = &mapToAlethicOperator);
std::expected<QMLExpression::Expression, std::string> parse(const std::string& formula, Parser::MappingFunction mappingFunction);

}
