/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 Abdullah Al Nahian Raiyan <abdullahal3829@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <lexer.h>
#include <definations.h>

#define isAl(x) (x >= 'A' && x <= 'Z' || x >= 'a' && x <= 'z')
#define isNum(x) (x >= '0' && x <= '9')
#define isId(x) (isAl(x) || isNum(x) || x == '_')

std::vector<TOKEN> Lexer::lex() {
    std::vector<TOKEN> tokens;

    while (!isAtEnd()) {
        char32_t c = code[idx];
        TOKEN tkn;
        tkn.row = row;
        tkn.col = col;
        tkn.value = c;

        switch (c) {
            case '(':
            case '[':
            case '{':
                tkn.type = TOKEN_LPAREN;
                break;

            case ')':
            case ']':
            case '}':
                tkn.type = TOKEN_RPAREN;
                break;

            case '+':
                if (idx + 1 < code.size() && code[idx + 1] == '=') {
                    tkn.type = TOKEN_INCREMENT;
                    advance();
                } else
                    tkn.type = TOKEN_PLUS;
                break;

            case '-':
                if (idx + 2 < code.size() && code[idx + 1] == '-' && code[idx + 2] == '-') {
                    advance();
                    advance();
                    advance();
                    bool closed = false;
                    while (!isAtEnd()) {
                        if (idx + 2 < code.size() && code[idx] == '-' && code[idx + 1] == '-' && code[idx + 2] == '-') {
                            advance();
                            advance();
                            advance();
                            closed = true;
                            break;
                        }
                        advance();
                    }
                    if (!closed)
                        error("Multiline comment is not closed using '---'.", row, col);
                    continue;
                } else if (idx + 1 < code.size() && code[idx + 1] == '=') {
                    tkn.type = TOKEN_DECREMENT;
                    advance();
                } else
                    tkn.type = TOKEN_MINUS;
                break;

            case '*':
                tkn.type = TOKEN_MULTIPLY;
                break;

            case '/':
                tkn.type = TOKEN_DIVIDE;
                break;

            case '=':
                if (idx + 1 < code.size() && code[idx + 1] == '=') {
                    advance();
                    tkn.type = TOKEN_EQUAL;
                } else
                    tkn.type = TOKEN_ASSIGNMENT;
                break;

            case '>':
                if (idx + 1 < code.size() && code[idx + 1] == '=') {
                    advance();
                    tkn.type = TOKEN_GREATER_EQUAL;
                } else
                    tkn.type = TOKEN_GREATER;
                break;

            case '<':
                if (idx + 1 < code.size() && code[idx + 1] == '=') {
                    advance();
                    tkn.type = TOKEN_SHORTER_EQUAL;
                } else
                    tkn.type = TOKEN_SHORTER;
                break;

            case '#': {
                advance();
                std::string text = "#";
                if (isAl(code[idx])) {
                    text += code[idx];
                    while (!isAtEnd() && (isAl(code[idx + 1]) || isNum(code[idx + 1]))) {
                        advance();
                        text += code[idx];
                    }
                    if (keywords.contains(text)) {
                        tkn.type = keywords.find(text)->second;
                        tkn.value = text;
                    } else {
                        while (code[idx - 1] != '\n' && !isAtEnd())
                            advance();
                        continue;
                    }
                } else {
                    while (code[idx - 1] != '\n' && !isAtEnd())
                        advance();
                    continue;
                }
                break;
            }

            case ':':
                tkn.type = TOKEN_COLON;
                break;

            case ',':
                tkn.type = TOKEN_COMMA;
                break;

            case ' ':
            case '\t':
            case '\r':
                advance();
                continue;
                
            case '\n':
                advance();
                continue;
                
            case ';':
                tkn.type = TOKEN_SEMICOLON;
                break;
                
            case '.':
                tkn.type = TOKEN_DOT;
                break;
                
            case '&':
                tkn.type = TOKEN_AND;
                break;
                
            case '|':
                tkn.type = TOKEN_OR;
                break;
                
            case '^':
                tkn.type = TOKEN_XOR;
                break;
                
            case '!':
                tkn.type = TOKEN_NOT;
                break;

            case '\'':
            case '"': {
                advance();
                std::string text;
                while (peek() != c) {
                    if (isAtEnd()) {
                        error(std::string("String literal is not closed with ") +
                            (c == '"' ? "\"" : "'" ) + ".", row, col);
                        break;
                    }
                    text += code[idx];
                    advance();
                }
                tkn.value = text;
                tkn.type = TOKEN_STRING;
                break;
            }

            default:
                std::string text;
                if (isAl(c) || c == '_') {
                    text += c;
                    while (!isAtEnd() && isId(code[idx + 1])) {
                        advance();
                        text += code[idx];
                    }
                    tkn.value = text;
                    if (keywords.contains(text))
                        tkn.type = keywords.find(text)->second;
                    else
                        tkn.type = TOKEN_IDENTIFIER;
                } else if (isNum(c)) {
                    text += c;
                    while (!isAtEnd() && isNum(code[idx + 1])) {
                        advance();
                        text += code[idx];
                    }
                    tkn.value = text;
                    tkn.type = TOKEN_NUMBER;
                } else {
                    advance();
                    continue;
                }
                break;
        }
        
        tokens.push_back(tkn);
        advance();
    }

    idx = 0;
    row = 1;
    col = 1;
    return tokens;
}

bool Lexer::isAtEnd() {
    return idx >= code.size();
}

void Lexer::advance() {
    col++;
    if (col > getLine(row).size() + 1) {
        col = 1;
        row++;
    }
    idx++;
}

char32_t Lexer::peek() {
    if (isAtEnd()) return '\0';
    return code[idx];
}