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
#include <vector>
#include <variant>
#include <lexer.h>
#include <parser.h>

struct FeatureMetadata {
    int row = 1, col = 1;
    bool state;
};

struct CompilerMetadata {
    std::map<std::string, FeatureMetadata> features;
    std::map<std::string, std::string> defines;
};

class Preprocessor {
private:
    int idx;
    std::vector<AST*> nodes;
    std::map<std::string, std::string> defines;
    bool inConditionalBlock = false;
    bool conditionalBlockActive = true;
    bool anyBranchTaken = false;
    
    void processIfWhileNode(IfWhileNode* node);
    void processNode(AST* node, CompilerMetadata* meta, std::vector<AST*>& out);
    void optimizeBinOpNode(BinOpNode* node);
    AST* evaluateConstantExpression(AST* node);
    bool isConstant(AST* node);
    bool isTruthy(AST* node);
    AST* substituteMacros(AST* node);
public:
    Preprocessor(std::vector<AST*> n) : nodes(std::move(n)), idx(0) { }
    std::vector<AST*> preprocess(CompilerMetadata* meta);
};