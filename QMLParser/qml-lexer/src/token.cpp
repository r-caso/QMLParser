/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "token.hpp"

namespace iif_sadaf::talk::QMLParser {

Token::Token(std::string literal, TokenType type)
    : literal(std::move(literal)), type(type)
{}

}