/*
 * SPDX-FileCopyrightText: 2024-2025 Ramiro Caso <caso.ramiro@conicet.gov.ar>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lexer.hpp"

#include <ranges>
#include <unordered_map>

#include <QMLExpression/expression.hpp>

namespace iif_sadaf::talk::QMLParser {

namespace {
    const std::unordered_map<uint8_t, std::unordered_map<char, uint8_t>> variable_dfa = {
        {0, { { 'x', 1 },
              { 'y', 1 },
              { 'z', 1 } }
        },
        {1, { { '_', 2 },
              { '0', 3 },
              { '1', 3 },
              { '2', 3 },
              { '3', 3 },
              { '4', 3 },
              { '5', 3 },
              { '6', 3 },
              { '7', 3 },
              { '8', 3 },
              { '9', 3 } }
        },
        {2, { { '0', 3 },
              { '1', 3 },
              { '2', 3 },
              { '3', 3 },
              { '4', 3 },
              { '5', 3 },
              { '6', 3 },
              { '7', 3 },
              { '8', 3 },
              { '9', 3 } }
        },
        {3, { { '0', 3 },
              { '1', 3 },
              { '2', 3 },
              { '3', 3 },
              { '4', 3 },
              { '5', 3 },
              { '6', 3 },
              { '7', 3 },
              { '8', 3 },
              { '9', 3 } }
        }
    };

    const uint8_t final_states[2] = {1, 3};
    const uint8_t initial_state = 0;

    bool isVariable(const std::string& token)
    {
        uint8_t state = initial_state;
        for (const char c : token) {
            if (!variable_dfa.at(state).contains(c)) {
                return false;
            }
            state = variable_dfa.at(state).at(c);
        }
        return state == final_states[0] || state == final_states[1];
    }

    void writeToTokenList(std::string& token, std::vector<Token>& list)
    {
        if (token.empty()) {
            return;
        }

        if (isVariable(token)) {
            list.emplace_back(token, TokenType::VARIABLE);
        }
        else {
            list.emplace_back(token, TokenType::IDENTIFIER);
        }
        token.clear();
    }

    void writeToTokenList(std::vector<uint8_t>& token_byte_array, TokenType type, std::vector<Token>& list)
    {
        if (token_byte_array.empty()) {
            return;
        }

        std::string token_str;
        for (uint8_t const c : token_byte_array) {
            token_str.push_back(c);
        }
        list.emplace_back(token_str, type);
        token_byte_array.clear();
    }

    bool isValidIdentifierSymbol(uint8_t c)
    {
        return c == '_'
            || c == '.'
            || (c >= '0' && c <= '9')
            || (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
        ;
    }
}

std::vector<Token> lex(const std::string& formula)
{
    std::vector<Token> list;
    std::string identifier;
    std::vector<uint8_t> operator_byte_array;

    const auto flush = [&](TokenType type) -> void {
        writeToTokenList(identifier, list);
        writeToTokenList(operator_byte_array, type, list);
    };
    const auto addToList = [&](const std::string& literal, TokenType type) -> void {
        flush(TokenType::ILLEGAL);
        list.emplace_back(literal, type);
    };
    const auto initOp = [&](uint8_t curr) -> void {
        flush(TokenType::ILLEGAL);
        operator_byte_array.push_back(curr);
    };
    const auto addToOp = [&](uint8_t curr, uint8_t prev) -> void {
        if (!operator_byte_array.empty() && operator_byte_array.back() == prev) {
            operator_byte_array.push_back(curr);
        }
        else {
            writeToTokenList(operator_byte_array, TokenType::ILLEGAL, list);
        }
    };
    const auto addToOpAndFlush = [&](uint8_t curr, uint8_t prev, TokenType type) -> void {
        if (!operator_byte_array.empty() && operator_byte_array.back() == prev) {
            operator_byte_array.push_back(curr);
            writeToTokenList(operator_byte_array, type, list);
        }
        else {
            writeToTokenList(operator_byte_array, TokenType::ILLEGAL, list);
        }
    };

    for (const uint8_t c : formula) {
        /*******************
         * SKIP WHITESPACE *
         *******************/

        if (c == ' ') {
            flush(TokenType::ILLEGAL);
            continue;
        }

        /***************
         * PUNCTUATION *
         ***************/

        else if (c == '(') {
            addToList("(", TokenType::LPAREN);
        }
        else if (c == ')') {
            addToList(")", TokenType::RPAREN);
        }
        else if (c == '[') {
            addToList("[", TokenType::LBRACKET);
        }
        else if (c == ']') {
            addToList("]", TokenType::RBRACKET);
        }
        else if (c == ',') {
            addToList(",", TokenType::COMMA);
        }

        /*********************
         * LOGICAL OPERATORS *
         *********************/

        /************
         * NEGATION *
         ************/

        else if (c == 0xc2) { // first byte of negation
            initOp(0xc2);
        }

        else if (c == 0xAC) { // second byte of negation
            addToOpAndFlush(0xAC, 0xc2, TokenType::NOT);
        }

        /*******************
         * OTHER OPERATORS *
         *******************/

        else if (c == 0xe2) { // first byte of all other operators
            initOp(0xe2);
        }

        /*******************************
         * IMPLICATION AND EQUIVALENCE *
         *******************************/

        else if (c == 0x86) { // second byte of implication and equivalence
            addToOp(0x86, 0xe2);
        }

        else if (c == 0x92) { // third byte of implication
            addToOpAndFlush(0x92, 0x86, TokenType::IF);
        }

        else if (c == 0x94) { // third byte of equivalence
            addToOpAndFlush(0x94, 0x86, TokenType::EQ);
        }

        /***************
         * QUANTIFIERS *
         ***************/

        else if (c == 0x88) { // second byte of forall, exists, not_exists, conjunction, disjunction
            addToOp(0x88, 0xe2);
        }

        else if (c == 0x80) { // third byte of forall
            addToOpAndFlush(0x80, 0x88, TokenType::FORALL);
        }

        else if (c == 0x83) { // third byte of exists
            addToOpAndFlush(0x83, 0x88, TokenType::EXISTS);
        }

        else if (c == 0x84) { // third byte of not_exists, third byte of possibility
            if (!operator_byte_array.empty()) {
                switch (operator_byte_array.back()) {
                case 0x88:
                    operator_byte_array.push_back(0x84);
                    writeToTokenList(operator_byte_array, TokenType::NOT_EXISTS, list);
                    break;
                case 0x8B:
                    operator_byte_array.push_back(0x84);
                    writeToTokenList(operator_byte_array, TokenType::POS, list);
                    break;
                default:
                    writeToTokenList(operator_byte_array, TokenType::ILLEGAL, list);
                    break;
                }
            }
            else {
                writeToTokenList(operator_byte_array, TokenType::ILLEGAL, list);
            }
        }

        else if (c == 0xA7) { // third byte of conjunction
            addToOpAndFlush(0xA7, 0x88, TokenType::AND);
        }

        else if (c == 0xA8) { // third byte of disjunction
            addToOpAndFlush(0xA8, 0x88, TokenType::OR);
        }

        /**************
         * INEQUALITY *
         **************/

        else if (c == 0x89) { // second byte of inequality
            addToOp(0x89, 0xe2);
        }

        else if (c == 0xA0) { // third byte of inequality
            addToOpAndFlush(0xA0, 0x89, TokenType::NEQ);
        }

        /******************
        * MODAL OPERATORS *
        *******************/

        else if (c == 0x8B) { // second byte of possibility
             addToOp(0x8B, 0xe2);
        }

        else if (c == 0x96) { // second byte of necessity
            addToOp(0x96, 0xe2);
        }

        else if (c == 0xA1) { // third byte of necessity
            addToOpAndFlush(0xA1, 0x96, TokenType::NEC);
        }


        /*******************
         * ALL IDENTIFIERS *
         *******************/

        else if (c == '=') {
            flush(TokenType::ILLEGAL);
            list.emplace_back(Token("=", TokenType::ID));
        }

        else if (isValidIdentifierSymbol(c)) {
            identifier.push_back(c);
        }

        else {
            list.emplace_back(Token(std::string(1, c), TokenType::ILLEGAL));
        }
    }

    flush(TokenType::ILLEGAL);

    list.emplace_back("EOI", TokenType::EOI);

    return list;
}

}
