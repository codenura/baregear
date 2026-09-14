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

#include <cstring>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <vector>
#include <variant>
#include <definations.h>
#include <lexer.h>
#include <parser.h>

#define T(i)            (tokens[i])
#define TT(i)           (tokens[i].type)
#define MATCH(i, t)     (tokens[i].type == t)
#define isMacro(i)      (T(i).value[0] == '#' && \
                         (MATCH(i, TOKEN_IF) || MATCH(i, TOKEN_ELSE) || \
                          MATCH(i, TOKEN_ELIF) || MATCH(i, TOKEN_DEFINE) || \
                          MATCH(i, TOKEN_IMPORTANCE) || MATCH(i, TOKEN_ERROR) || \
                          MATCH(i, TOKEN_WARNING) || MATCH(i, TOKEN_C) || \
                          MATCH(i, TOKEN_ASM) || MATCH(i, TOKEN_FEATURE) || \
                          MATCH(i, TOKEN_NO_FEATURE) || MATCH(i, TOKEN_END)))

std::vector<AST*> Parser::parse() {
    std::vector<AST*> nodes;
    while (!isAtEnd()) {
        auto stmts = statement();
        nodes.insert(nodes.end(), stmts.begin(), stmts.end());
    }
    tokens.clear();
    lexs.clear();
    return nodes;
}

std::vector<AST*> Parser::statement() {
    std::vector<AST*> nodes;
    TOKEN_TYPE current = TT(idx);

    if (current == TOKEN_DEFINE) {
        int startRow = T(idx).row, startCol = T(idx).col;
        idx++;
        std::string defineName = T(idx).value;
        idx++;
        std::string defineValue = "";
        // Collect value until end of line or next macro
        while (!isAtEnd() && TT(idx) != TOKEN_IF && TT(idx) != TOKEN_ELSE &&
               TT(idx) != TOKEN_FEATURE && TT(idx) != TOKEN_NO_FEATURE &&
               TT(idx) != TOKEN_DEFINE && TT(idx) != TOKEN_C && TT(idx) != TOKEN_ASM &&
               TT(idx) != TOKEN_END && T(idx).row == T(idx - 1).row) {
            defineValue += T(idx).value + " ";
            idx++;
        }
        // Remove trailing space if present
        if (!defineValue.empty() && defineValue.back() == ' ')
            defineValue.pop_back();

        nodes.push_back(new DefineNode(defineName, defineValue, startRow, startCol));
    } else if (current == TOKEN_FEATURE) {
        int featRow = T(idx).row, featCol = T(idx).col;
        idx++;
        std::string featureName = T(idx).value;
        idx++;
        nodes.push_back(new FeatureNode(featureName, true, featRow, featCol));
    } else if (current == TOKEN_NO_FEATURE) {
        int noFeatRow = T(idx).row, noFeatCol = T(idx).col;
        idx++;
        std::string featureName = T(idx).value;
        idx++;
        nodes.push_back(new FeatureNode(featureName, false, noFeatRow, noFeatCol));
    } else if (current == TOKEN_C || current == TOKEN_ASM) {
        int codeRow = T(idx).row, codeCol = T(idx).col;
        TOKEN_TYPE lang = current;
        idx++;
        std::string code = "";
        
        // Multi-line block: code begins on a line after the marker and runs until
        // #end at the same or a lesser column than the marker
        if (!isAtEnd() && T(idx).row != codeRow) {
            while (!isAtEnd() && !(TT(idx) == TOKEN_END && T(idx).col <= codeCol)) {
                code += T(idx).value + " ";
                idx++;
            }
            // Skip the #end token
            if (!isAtEnd() && TT(idx) == TOKEN_END) {
                nodes.push_back(new EndNode(lang, codeRow, codeCol));
                idx++;
            }
        } else {
            // Single-line inline code
            while (!isAtEnd() && TT(idx) != TOKEN_IF && TT(idx) != TOKEN_ELSE && 
                   TT(idx) != TOKEN_FEATURE && TT(idx) != TOKEN_NO_FEATURE &&
                   TT(idx) != TOKEN_C && TT(idx) != TOKEN_ASM &&
                   TT(idx) != TOKEN_END && T(idx).row == T(idx - 1).row) {
                code += T(idx).value + " ";
                idx++;
            }
        }
        nodes.push_back(new InlineCodeNode(code, lang, codeRow, codeCol));
    } else if (current == TOKEN_END) {
        int endRow = T(idx).row, endCol = T(idx).col;
        idx++;
        nodes.push_back(new EndNode(TOKEN_END, endRow, endCol));
    } else if (current == TOKEN_IF) {
        int prevIDX = idx;
        int ifRow = T(idx).row, ifCol = T(idx).col;
        idx++;
        AST* condition = expr();
        if (T(prevIDX).value[0] == '#')
            nodes.push_back(new IfWhileNode(condition, parseBody(ifCol), TOKEN_IF, '#', ifRow, ifCol));
        else if (MATCH(idx, TOKEN_COLON)) {
            idx++;
            std::vector<AST*> ifBody = parseBody(ifCol);
            nodes.push_back(new IfWhileNode(condition, ifBody, TOKEN_IF, ifRow, ifCol));
        } else
            error("Expected ':' after if condition", ifRow, ifCol);
    } else if (current == TOKEN_ELSE) {
        int prevIDX = idx;
        int elseRow = T(idx).row, elseCol = T(idx).col;
        if (MATCH(idx + 1, TOKEN_IF)) {
            idx++;
            idx++;
            AST* condition = expr();
            if (T(prevIDX).value[0] == '#') {
                nodes.push_back(new IfWhileNode(condition, parseBody(elseCol), TOKEN_ELIF, '#', elseRow, elseCol));
            } else if (MATCH(idx, TOKEN_COLON)) {
                idx++;
                std::vector<AST*> elseIfBody = parseBody(elseCol);
                nodes.push_back(new IfWhileNode(condition, elseIfBody, TOKEN_ELIF, elseRow, elseCol));
            } else
                error("Expected ':' after else if condition", elseRow, elseCol);
        } else {
            idx++;
            if (T(prevIDX).value[0] == '#') {
                nodes.push_back(new IfWhileNode(nullptr, parseBody(elseCol), TOKEN_ELSE, '#', elseRow, elseCol));
            } else if (MATCH(idx, TOKEN_COLON)) {
                idx++;
                std::vector<AST*> elseBody = parseBody(elseCol);
                nodes.push_back(new IfWhileNode(nullptr, elseBody, TOKEN_ELSE, elseRow, elseCol));
            } else {
                error("Expected ':' after else", elseRow, elseCol);
            }
        }
    } else if (current == TOKEN_ELIF) {
        int prevIDX = idx;
        int elifRow = T(idx).row, elifCol = T(idx).col;
        idx++;
        AST* condition = expr();
        if (T(prevIDX).value[0] == '#') {
            nodes.push_back(new IfWhileNode(condition, parseBody(elifCol), TOKEN_ELIF, '#', elifRow, elifCol));
        } else if (MATCH(idx, TOKEN_COLON)) {
            idx++;
            std::vector<AST*> elifBody = parseBody(elifCol);
            nodes.push_back(new IfWhileNode(condition, elifBody, TOKEN_ELIF, elifRow, elifCol));
        } else
            error("Expected ':' after elif condition", elifRow, elifCol);
    } else if (current == TOKEN_WHILE) {
        int whileRow = T(idx).row, whileCol = T(idx).col;
        idx++;
        AST* condition = expr();
        if (MATCH(idx, TOKEN_COLON)) {
            idx++;
            std::vector<AST*> whileBody = parseBody(whileCol);
            nodes.push_back(new IfWhileNode(condition, whileBody, TOKEN_WHILE, whileRow, whileCol));
        } else
            error("Expected ':' after while condition", whileRow, whileCol);
    } else if (current == TOKEN_FOR) {
        int forRow = T(idx).row, forCol = T(idx).col;
        idx++; // skip 'for'
        while (!isAtEnd() && !MATCH(idx, TOKEN_COLON))
            idx++;
        if (!isAtEnd() && MATCH(idx, TOKEN_COLON))
            idx++;
        while (!isAtEnd() && T(idx).col > forCol)
            for (AST* s : statement())
                nodes.push_back(s);
    } else if (current == TOKEN_SWITCH) {
        int swRow = T(idx).row, swCol = T(idx).col;
        idx++;
        AST* condition = expr();
        if (!MATCH(idx, TOKEN_COLON)) {
            error("Expected ':' after switch condition", swRow, swCol);
        } else {
            idx++;
            std::vector<AST*> cases;
            while (!isAtEnd() && T(idx).col > swCol) {
                if (TT(idx) == TOKEN_CASE || TT(idx) == TOKEN_DEFAULT_CASE) {
                    int caseRow = T(idx).row, caseCol = T(idx).col;
                    bool isDefault = TT(idx) == TOKEN_DEFAULT_CASE;
                    idx++;
                    AST* caseValue = nullptr;
                    if (!isDefault)
                        caseValue = expr();
                    if (!MATCH(idx, TOKEN_COLON)) {
                        error("Expected ':' after case", caseRow, caseCol);
                        break;
                    }
                    idx++;
                    std::vector<AST*> caseBody;
                    while (!isAtEnd() && T(idx).col > caseCol)
                        for (AST* s : statement())
                            caseBody.push_back(s);
                    cases.push_back(new CaseNode(caseValue, isDefault, caseBody, caseRow, caseCol));
                } else {
                    idx++;
                }
            }
            nodes.push_back(new SwitchNode(condition, cases, swRow, swCol));
        }
    } else if (current == TOKEN_RETURN) {
        int retRow = T(idx).row, retCol = T(idx).col;
        idx++;
        AST* returnValue = nullptr;
        if (!isAtEnd() && T(idx).row == retRow)
            returnValue = expr();
        nodes.push_back(new ReturnNode(returnValue, retRow, retCol));
        return nodes;
    } else if (current == TOKEN_VAR || current == TOKEN_INT || current == TOKEN_FLOAT ||
               current == TOKEN_DOUBLE || current == TOKEN_SHORT || current == TOKEN_NUMB ||
               current == TOKEN_TEXT) {
        DATATYPE dtype;
        switch (current) {
            case TOKEN_BOOLEAN: dtype = BOOL; break;
            case TOKEN_INT:     dtype = INT;    break;
            case TOKEN_FLOAT:   dtype = FLOAT;  break;
            case TOKEN_DOUBLE:  dtype = DOUBLE; break;
            case TOKEN_SHORT:   dtype = SHORT;  break;
            case TOKEN_NUMB:    dtype = NUMBER; break;
            case TOKEN_TEXT:    dtype = STRING; break;
            default:            dtype = VARIANT; break;
        }
        int declRow = T(idx).row, declCol = T(idx).col;
        idx++;
        std::vector<std::string> names;
        while (!isAtEnd() && TT(idx) == TOKEN_IDENTIFIER) {
            names.push_back(T(idx).value);
            idx++;
            if (!isAtEnd() && MATCH(idx, TOKEN_COMMA)) {
                idx++;
                continue;
            }
            break;
        }
        if (!names.empty() && !isAtEnd() && MATCH(idx, TOKEN_COLON) && idx + 1 < tokens.size()) {
            switch (TT(idx + 1)) {
                case TOKEN_BOOLEAN: dtype = BOOL; break;
                case TOKEN_INT:     dtype = INT;    break;
                case TOKEN_FLOAT:   dtype = FLOAT;  break;
                case TOKEN_DOUBLE:  dtype = DOUBLE; break;
                case TOKEN_SHORT:   dtype = SHORT;  break;
                case TOKEN_NUMB:    dtype = NUMBER; break;
                case TOKEN_TEXT:    dtype = STRING; break;
                default:            break;
            }
            idx += 2;
        }
        for (const auto& name : names)
            nodes.push_back(new VariableNode(name, dtype, true, declRow, declCol));
        if (!names.empty() && !isAtEnd() && MATCH(idx, TOKEN_ASSIGNMENT)) {
            idx++;
            AST* value = expr();
            for (const auto& name : names)
                nodes.push_back(new AssignNode(name, value, declRow, declCol));
        }
    } else if (current == TOKEN_IDENTIFIER) {
        if (isFunctionDefinition()) {
            int defIdx = idx;
            std::string funcName = T(idx).value;
            idx++;
            std::vector<VariableNode*> params;
            if (MATCH(idx, TOKEN_COLON)) {
                idx++;
            } else {
                while (!isAtEnd() && !MATCH(idx, TOKEN_COLON)) {
                    std::string pName = T(idx).value;
                    DATATYPE dtype = VARIANT;
                    idx++;
                    if (!isAtEnd() && T(idx).value == "as" && idx + 1 < tokens.size()) {
                        switch (TT(idx + 1)) {
                            case TOKEN_INT:    dtype = INT;    break;
                            case TOKEN_FLOAT:  dtype = FLOAT;  break;
                            case TOKEN_DOUBLE: dtype = DOUBLE; break;
                            case TOKEN_SHORT:  dtype = SHORT;  break;
                            case TOKEN_NUMB:   dtype = NUMBER; break;
                            case TOKEN_TEXT:   dtype = STRING; break;
                            default: break;
                        }
                        idx += 2;
                    }
                    if (!isAtEnd() && MATCH(idx, TOKEN_MINUS) && idx + 1 < tokens.size() &&
                        T(idx + 1).value == "optional") {
                        idx += 2;
                    }
                    params.push_back(new VariableNode(pName, dtype, false, T(defIdx).row, T(defIdx).col));
                    if (!isAtEnd() && MATCH(idx, TOKEN_COMMA)) {
                        idx++;
                        continue;
                    }
                    break;
                }
            }
            if (!isAtEnd() && MATCH(idx, TOKEN_COLON))
                idx++;
            int startCol = T(defIdx).col;
            std::vector<AST*> body;
            while (!isAtEnd() && (T(idx).col > startCol || isMacro(idx)))
                for (AST* s : statement())
                    body.push_back(s);
            nodes.push_back(new FunctionNode(funcName, params, body, T(defIdx).row, T(defIdx).col));
            return nodes;
        }
        std::vector<std::string> varNames;
        int saveIdx = idx;

        while (true) {
            if (!isAtEnd() && TT(idx) == TOKEN_IDENTIFIER) {
                varNames.push_back(T(idx).value);
                idx++;
                if (!isAtEnd() && MATCH(idx, TOKEN_COMMA)) {
                    idx++;
                    continue;
                } else if (!isAtEnd() && MATCH(idx, TOKEN_ASSIGNMENT)) {
                    idx++;
                    break;
                } else {
                    TOKEN_TYPE next = isAtEnd() ? TOKEN_SEMICOLON : TT(idx);
                    if (next == TOKEN_COLON && idx + 1 < tokens.size() &&
                        (TT(idx + 1) == TOKEN_INT || TT(idx + 1) == TOKEN_FLOAT ||
                         TT(idx + 1) == TOKEN_DOUBLE || TT(idx + 1) == TOKEN_SHORT ||
                         TT(idx + 1) == TOKEN_NUMB || TT(idx + 1) == TOKEN_TEXT)) {
                        DATATYPE dtype = VARIANT;
                        switch (TT(idx + 1)) {
                            case TOKEN_INT:    dtype = INT;    break;
                            case TOKEN_FLOAT:  dtype = FLOAT;  break;
                            case TOKEN_DOUBLE: dtype = DOUBLE; break;
                            case TOKEN_SHORT:  dtype = SHORT;  break;
                            case TOKEN_NUMB:   dtype = NUMBER; break;
                            case TOKEN_TEXT:   dtype = STRING; break;
                            default: break;
                        }
                        idx += 2;
                        for (const auto& varName : varNames)
                            nodes.push_back(new VariableNode(varName, dtype, true, T(saveIdx).row, T(saveIdx).col));
                        if (!isAtEnd() && MATCH(idx, TOKEN_ASSIGNMENT)) {
                            idx++;
                            AST* value = expr();
                            for (const auto& varName : varNames)
                                nodes.push_back(new AssignNode(varName, value, T(saveIdx).row, T(saveIdx).col));
                        }
                    } else {
                        idx = saveIdx;
                        nodes.push_back(expr());
                    }
                    break;
                }
            } else {
                if (isAtEnd())
                    break;
                idx = saveIdx;
                nodes.push_back(expr());
                break;
            }
        }

        if (!varNames.empty() && idx > 0 && TT(idx - 1) == TOKEN_ASSIGNMENT) {
            for (const auto& varName : varNames)
                nodes.push_back(new VariableNode(varName, VARIANT, true, T(saveIdx).row, T(saveIdx).col));
            AST* value = expr();
            for (const auto& varName : varNames)
                nodes.push_back(new AssignNode(varName, value, T(saveIdx).row, T(saveIdx).col));
        }
    } else if (current == TOKEN_IMPORTANCE) {
        const int prevIdx = idx;
        idx++;
        std::string lowercase = T(idx).value;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        STMTImportance stmti;
        if (lowercase == "low")
            stmti = IMPORTANCE_LOW;
        else if (lowercase == "medium")
            stmti = IMPORTANCE_MEDIUM;
        else if (lowercase == "high")
            stmti = IMPORTANCE_HIGH;
        else if (lowercase == "urgent")
            stmti = IMPORTANCE_URGENT;
        else {
            error(lowercase + " is not supported importance level", T(idx).row, T(idx).col);
            return nodes;
        }
        idx++;
        std::vector<AST*> body;
        while ((T(idx).col > T(prevIdx + 1).col || isMacro(idx)) && !isAtEnd()) {
            if (isMacro(idx))
                for (AST* s : statement())
                    body.push_back(s);
            else
                body.push_back(expr());
        }

        nodes.push_back(new ImportanceNode(body, stmti, T(idx).row, T(idx).col));
    } else if (current == TOKEN_CONDITION) {
        const int prevIdx = idx;
        idx++;
        std::vector<AST*> body;
        while (!isAtEnd() && (T(prevIdx).col > T(prevIdx + 1).col || isMacro(idx)))
            for (AST* s : statement())
                body.push_back(s);
        nodes.push_back(new ConditionalFlagNode(expr(), body, T(idx).row, T(idx).col));
    }
    else if (current == TOKEN_NOCHANGE) {
        const int prevIdx = idx;
        idx++;
        std::vector<AST*> body;
        while (!isAtEnd() && (T(prevIdx).col > T(prevIdx + 1).col || isMacro(idx)))
            for (AST* s : statement())
                body.push_back(s);
        nodes.push_back(new NoChangeNode(body, T(idx).row, T(idx).col));
    } else
        if (!isAtEnd())
            nodes.push_back(expr());

    return nodes;
}

std::vector<AST*> Parser::parseBody(unsigned int baseCol) {
    std::vector<AST*> body;
    while (!isAtEnd() && TT(idx) != TOKEN_SEMICOLON && T(idx).col > baseCol) {
        for (AST* s : statement())
            body.push_back(s);
    }
    return body;
}

bool Parser::isFunctionDefinition() {
    int i = idx;
    if (i >= tokens.size() || TT(i) != TOKEN_IDENTIFIER)
        return false;
    int nameRow = T(i).row;
    i++;
    if (i >= tokens.size())
        return false;
    if (TT(i) == TOKEN_COLON)
        return i + 1 >= tokens.size() || T(i + 1).row > T(i).row;
    if (T(i).row != nameRow)
        return false;
    while (i < tokens.size()) {
        if (T(i).row != nameRow)
            return false;
        if (TT(i) != TOKEN_IDENTIFIER)
            return false;
        i++;
        if (i < tokens.size() && T(i).value == "as") {
            i++;
            if (i < tokens.size() && (TT(i) == TOKEN_INT || TT(i) == TOKEN_FLOAT ||
                                      TT(i) == TOKEN_DOUBLE || TT(i) == TOKEN_SHORT ||
                                      TT(i) == TOKEN_NUMB || TT(i) == TOKEN_TEXT)) {
                i++;
            } else {
                return false;
            }
        }
        if (i < tokens.size() && MATCH(i, TOKEN_MINUS)) {
            i++;
            if (i < tokens.size() && T(i).value == "optional")
                i++;
            else
                return false;
        }
        if (i < tokens.size()) {
            if (TT(i) == TOKEN_COLON)
                return i + 1 >= tokens.size() || T(i + 1).row > T(i).row;
            if (TT(i) == TOKEN_COMMA) {
                i++;
                continue;
            }
            return false;
        }
    }
    return false;
}

AST* Parser::expr() {
    AST* node = term();
    while (!isAtEnd() && (
           MATCH(idx, TOKEN_PLUS) || MATCH(idx, TOKEN_MINUS) ||
           MATCH(idx, TOKEN_EQUAL) || MATCH(idx, TOKEN_GREATER) ||
           MATCH(idx, TOKEN_SHORTER) || MATCH(idx, TOKEN_GREATER_EQUAL) ||
           MATCH(idx, TOKEN_SHORTER_EQUAL) || MATCH(idx, TOKEN_AND) ||
           MATCH(idx, TOKEN_OR) || MATCH(idx, TOKEN_XOR) ||
           MATCH(idx, TOKEN_ASSIGNMENT) || MATCH(idx, TOKEN_INCREMENT) ||
           MATCH(idx, TOKEN_DECREMENT))) {
        TOKEN_TYPE op = TT(idx);
        int opRow = T(idx).row, opCol = T(idx).col;
        idx++;
        if (op == TOKEN_ASSIGNMENT) {
            if (auto* var = dynamic_cast<VariableNode*>(node)) {
                node = new AssignNode(var->name, expr(), opRow, opCol);
                break;
            }
            continue;
        }
        else if (op == TOKEN_INCREMENT || op == TOKEN_DECREMENT) {
            if (auto* var = dynamic_cast<VariableNode*>(node)) {
                node = new BinOpNode(var, op, term(), opRow, opCol);
                break;
            }
            continue;
        }
        node = new BinOpNode(node, op, term(), opRow, opCol);
    }
    return node;
}

AST* Parser::term() {
    AST* node = factor();
    while (!isAtEnd() && (MATCH(idx, TOKEN_MULTIPLY) || MATCH(idx, TOKEN_DIVIDE))) {
        TOKEN_TYPE op = TT(idx);
        int opRow = T(idx).row, opCol = T(idx).col;
        idx++;
        node = new BinOpNode(node, op, factor(), opRow, opCol);
    }
    return node;
}

AST* Parser::factor() {
    TOKEN_TYPE op = TT(idx);
    if (op == TOKEN_STRING || op == TOKEN_NUMBER || op == TOKEN_TRUE || op == TOKEN_FALSE) {
        idx++;
        if (op == TOKEN_TRUE || op == TOKEN_FALSE) {
            if (op == TOKEN_TRUE)
                return new BooleanNode(true, T(idx - 1).row, T(idx - 1).col);
            else if (op == TOKEN_FALSE)
                return new BooleanNode(false, T(idx - 1).row, T(idx - 1).col);
            else
                error("Boolean Value Must Be 'true' or 'false'.", T(idx).row, T(idx).col);
        }
            
        return new ValueNode(T(idx - 1).value, T(idx - 1).row, T(idx - 1).col);
    }
    else if (op == TOKEN_IDENTIFIER) {
        // Check if this is a function definition (followed by colon)
        int saveIdx = idx;
        idx++;
        if (idx < tokens.size() && MATCH(idx, TOKEN_LPAREN)) {
            // Function call with arguments: func(arg1, arg2)
            idx = saveIdx;
            std::string funcName = T(idx).value;
            idx++; // skip function name
            idx++; // skip opening paren
            
            std::vector<AST*> args;
            while (!MATCH(idx, TOKEN_RPAREN) && !isAtEnd()) {
                args.push_back(expr());
                if (MATCH(idx, TOKEN_COMMA))
                    idx++;
                else
                    error("Expected ','.", T(idx).row, T(idx).col);
            }
            idx++; // skip closing paren

            return new CallNode(funcName, args, T(saveIdx).row, T(saveIdx).col);
        }
        else {
            // Check if this is a function call without parentheses: func arg1, arg2
            // Look ahead to see if next token is a valid argument on the same line (identifier, string, number)
            int nextIdx = idx;
            bool sameRowArg = nextIdx < tokens.size() && T(nextIdx).row == T(saveIdx).row &&
                              (TT(nextIdx) == TOKEN_IDENTIFIER || TT(nextIdx) == TOKEN_STRING || TT(nextIdx) == TOKEN_NUMBER);
            bool loneIdentifier = (nextIdx >= tokens.size() || T(nextIdx).row != T(saveIdx).row) &&
                                  (saveIdx == 0 || T(saveIdx - 1).row != T(saveIdx).row);
            if (sameRowArg || loneIdentifier) {
                idx = saveIdx;
                std::string funcName = T(idx).value;
                idx++; // skip function name
                
                std::vector<AST*> args;
                // Collect arguments until end of line, semicolon, dot, or colon mismatch
                while (!isAtEnd() && TT(idx) != TOKEN_SEMICOLON && TT(idx) != TOKEN_DOT && T(idx).row == T(saveIdx).row) {
                    // Check for colon mismatch
                    if (TT(idx) == TOKEN_COLON && (idx + 1 >= tokens.size() || TT(idx + 1) != TOKEN_COLON))
                        break;
                    if (TT(idx) == TOKEN_COMMA) {
                        idx++;
                        continue;
                    }
                    args.push_back(expr());
                }
                
                return new CallNode(funcName, args, T(saveIdx).row, T(saveIdx).col);
            }
        }
        idx = saveIdx;
        idx++;
        return new VariableNode(T(idx - 1).value, VARIANT, false, T(idx - 1).row, T(idx - 1).col);
    } else if (op == TOKEN_ASSIGNMENT) {
        int assignRow = T(idx - 1).row, assignCol = T(idx - 1).col;
        const std::string name = T(idx - 1).value;
        idx++;
        return new AssignNode(name, factor(), assignRow, assignCol);
    }
    else if (op == TOKEN_LPAREN) {
        char openBracket = '(';
        if (!T(idx).value.empty())
            openBracket = T(idx).value[0];
        char closeBracket;
        
        switch (openBracket) {
            case '(': closeBracket = ')'; break;
            case '[': closeBracket = ']'; break;
            case '{': closeBracket = '}'; break;
            default: break;
        }
        
        idx++; // skip opening bracket
        AST* innerExpr = expr();

        if (MATCH(idx, TOKEN_RPAREN) == false && (closeBracket == ')' || closeBracket == ']' || closeBracket == '}')) {
            error("Expected closing bracket '" + std::string(1, closeBracket) + "'", T(idx).row, T(idx).col);
            return nullptr;
        }
        
        idx++; // skip closing bracket
        return innerExpr;
    }
    else if (op == TOKEN_DEFINED) {
        int saveIdx = idx;
        idx++;
        if (MATCH(idx, TOKEN_IDENTIFIER)) {
            int mRow = T(idx).row, mCol = T(idx).col;
            std::string macroName = T(idx).value;
            idx++;
            return new BinOpNode(new VariableNode(macroName, VARIANT, false, mRow, mCol),
                                 TOKEN_DEFINED, nullptr, mRow, mCol);
        }
        idx = saveIdx;
        idx++;
        return nullptr;
    }
    
    idx++;
    return nullptr;
}