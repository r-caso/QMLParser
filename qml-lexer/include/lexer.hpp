/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace iif_sadaf::talk::QMLParser {

std::vector<Token> lex(const std::string& string);

}