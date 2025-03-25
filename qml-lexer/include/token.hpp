/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

namespace iif_sadaf::talk::QMLParser {

/**
 * @enum TokenType
 * @brief Represents different types of tokens used in the QMLParser.
 */
enum class TokenType : uint8_t {
    NIL, EOI, ILLEGAL,                          // parsing-related
    NOT, AND, OR, IF, EQ,                       // Logical operators
    NEC, POS,                                   // modal operators
    FORALL, EXISTS, NOT_EXISTS,                 // quantifiers
    ID, NEQ,                                    // identity and inequality
    VARIABLE,                                   // variables
    IDENTIFIER,                                 // everything else
    LPAREN, RPAREN, LBRACKET, RBRACKET, COMMA,  // punctuation
};

/**
 * @struct Token
 * @brief Represents a single lexical token.
 */
struct Token {
    Token(std::string literal, TokenType type);

    std::string literal;
    TokenType type;
};

}