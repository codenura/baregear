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
#include <parser.h>
#include <preprocessor.h>
#include <definations.h>

extern bool errorHappens;
std::vector<AST*> Preprocessor::preprocess(CompilerMetadata* meta) {
    if (errorHappens) {
        failure();
        std::exit(1);
    }

    if (meta)
        defines = meta->defines;

    std::vector<AST*> optimizedNodes;
    for (idx = 0; idx < nodes.size(); idx++)
        processNode(nodes[idx], meta, optimizedNodes);

    nodes.clear();
    return optimizedNodes;
}

void Preprocessor::processNode(AST* node, CompilerMetadata* meta, std::vector<AST*>& out) {
    // Handle conditional directives first
    if (auto* ifNode = dynamic_cast<IfWhileNode*>(node)) {
        if (ifNode->prefix == '#') {
            processIfWhileNode(ifNode);
            if (conditionalBlockActive)
                for (AST* body : ifNode->body)
                    out.push_back(substituteMacros(body));
            return;
        }
        out.push_back(substituteMacros(node));
        return;
    }

    // #end resets the conditional block state
    if (auto* endNode = dynamic_cast<EndNode*>(node)) {
        if (endNode->endType == TOKEN_END) {
            inConditionalBlock = false;
            conditionalBlockActive = true;
            anyBranchTaken = false;
            return;
        }
        out.push_back(node);
        return;
    }

    // Skip non-directive content inside inactive conditional blocks
    if (!conditionalBlockActive && inConditionalBlock)
        return;

    // Define nodes for constants/macros - record the macro and replace usages
    if (auto* defineNode = dynamic_cast<DefineNode*>(node)) {
        defines[defineNode->name] = defineNode->value;
        return;
    } else if (auto* fn = dynamic_cast<FunctionNode*>(node)) {
        // Resolve conditionals inside function bodies in a fresh block scope
        bool savedInBlock = inConditionalBlock;
        bool savedActive = conditionalBlockActive;
        bool savedTaken = anyBranchTaken;
        inConditionalBlock = false;
        conditionalBlockActive = true;
        anyBranchTaken = false;
        std::vector<AST*> newBody;
        for (AST* b : fn->body)
            processNode(b, meta, newBody);
        fn->body = newBody;
        inConditionalBlock = savedInBlock;
        conditionalBlockActive = savedActive;
        anyBranchTaken = savedTaken;
        out.push_back(fn);
        return;
    } else if (auto* binOpNode = dynamic_cast<BinOpNode*>(node)) {
        optimizeBinOpNode(binOpNode);
        out.push_back(substituteMacros(node));
    } else if (auto* featureNode = dynamic_cast<FeatureNode*>(node)) {
        if (meta && meta->features.contains(featureNode->featureName)) {
            if (meta->features.find(featureNode->featureName)->second.state != featureNode->enabled) {
                if (meta->features.contains(featureNode->featureName))
                    warn(featureNode->enabled ? "Enabling Feature Again." : "Disabling Feature Again.",
                            featureNode->row, featureNode->col);

                FeatureMetadata fmeta;
                fmeta.row = featureNode->row;
                fmeta.col = featureNode->col;
                fmeta.state = featureNode->enabled;
                meta->features.insert({ featureNode->featureName, fmeta });
            }
        }
    } else if (dynamic_cast<InlineCodeNode*>(node) || dynamic_cast<ConditionalFlagNode*>(node) || dynamic_cast<NoChangeNode*>(node))
        out.push_back(node);
    else
        out.push_back(substituteMacros(node));
}

void Preprocessor::processIfWhileNode(IfWhileNode* node) {
    if (node->prefix != '#')
        return;
    if (node->sign == TOKEN_IF) {
        AST* result = node->condition ? evaluateConstantExpression(node->condition) : nullptr;
        bool taken = result && isConstant(result) && isTruthy(result);
        inConditionalBlock = true;
        anyBranchTaken = taken;
        conditionalBlockActive = taken;
    } else if (node->sign == TOKEN_ELIF) {
        if (!inConditionalBlock)
            return;
        if (anyBranchTaken) {
            conditionalBlockActive = false;
            return;
        }
        AST* result = node->condition ? evaluateConstantExpression(node->condition) : nullptr;
        bool taken = result && isConstant(result) && isTruthy(result);
        conditionalBlockActive = taken;
        anyBranchTaken = anyBranchTaken || taken;
    } else if (node->sign == TOKEN_ELSE) {
        if (!inConditionalBlock)
            return;
        conditionalBlockActive = !anyBranchTaken;
        anyBranchTaken = true;
    }
}

bool Preprocessor::isTruthy(AST* node) {
    auto* val = dynamic_cast<ValueNode*>(node);
    return val && val->value != "0";
}

AST* Preprocessor::substituteMacros(AST* node) {
    if (!node) return nullptr;
    if (auto* fn = dynamic_cast<FunctionNode*>(node)) {
        for (auto& b : fn->body) b = substituteMacros(b);
        return node;
    }
    if (auto* call = dynamic_cast<CallNode*>(node)) {
        for (auto& p : call->param) p = substituteMacros(p);
        return node;
    }
    if (auto* var = dynamic_cast<VariableNode*>(node)) {
        if (!var->isDecl && defines.contains(var->name))
            return new ValueNode(defines[var->name], var->row, var->col);
        return node;
    }
    if (auto* bin = dynamic_cast<BinOpNode*>(node)) {
        bin->left = substituteMacros(bin->left);
        bin->right = substituteMacros(bin->right);
        return node;
    }
    if (auto* asn = dynamic_cast<AssignNode*>(node)) {
        asn->node = substituteMacros(asn->node);
        return node;
    }
    if (auto* iw = dynamic_cast<IfWhileNode*>(node)) {
        iw->condition = substituteMacros(iw->condition);
        for (auto& b : iw->body) b = substituteMacros(b);
        return node;
    }
    return node;
}

void Preprocessor::optimizeBinOpNode(BinOpNode* node) {
    // Constant folding: evaluate constant expressions at compile time
    if (isConstant(node->left) && isConstant(node->right)) {
        AST* result = evaluateConstantExpression(node);
        if (result) {
            // Replace the BinOpNode with the constant result
            // Note: In a real implementation, you'd need proper memory management here
            // For now, we'll update the node to be a ValueNode by copying the result
            auto* resultVal = dynamic_cast<ValueNode*>(result);
            if (resultVal) {
                node->left = result;
                node->right = nullptr;
                node->op = TOKEN_NUMBER;
            }
        }
    }
}

AST* Preprocessor::evaluateConstantExpression(AST* node) {
    if (auto* valNode = dynamic_cast<ValueNode*>(node)) {
        return valNode;
    }
    
    if (auto* binOpNode = dynamic_cast<BinOpNode*>(node)) {
        if (binOpNode->op == TOKEN_DEFINED) {
            if (auto* var = dynamic_cast<VariableNode*>(binOpNode->left))
                return new ValueNode(defines.contains(var->name) ? "1" : "0");
            return nullptr;
        }

        AST* left = evaluateConstantExpression(binOpNode->left);
        AST* right = evaluateConstantExpression(binOpNode->right);
        
        if (left && right && isConstant(left) && isConstant(right)) {
            auto* leftVal = dynamic_cast<ValueNode*>(left);
            auto* rightVal = dynamic_cast<ValueNode*>(right);
            
            // Try to parse as double for arithmetic operations
            try {
                double leftNum = std::stod(leftVal->value);
                double rightNum = std::stod(rightVal->value);
                double result = 0.0;
                
                switch (binOpNode->op) {
                    case TOKEN_PLUS:
                        result = leftNum + rightNum;
                        break;
                    case TOKEN_MINUS:
                        result = leftNum - rightNum;
                        break;
                    case TOKEN_MULTIPLY:
                        result = leftNum * rightNum;
                        break;
                    case TOKEN_DIVIDE:
                        if (rightNum != 0) {
                            result = leftNum / rightNum;
                        } else {
                            return nullptr; // Division by zero
                        }
                        break;
                    case TOKEN_EQUAL:
                        return new ValueNode(leftNum == rightNum ? "1" : "0");
                    case TOKEN_GREATER:
                        return new ValueNode(leftNum > rightNum ? "1" : "0");
                    case TOKEN_SHORTER:
                        return new ValueNode(leftNum < rightNum ? "1" : "0");
                    case TOKEN_GREATER_EQUAL:
                        return new ValueNode(leftNum >= rightNum ? "1" : "0");
                    case TOKEN_SHORTER_EQUAL:
                        return new ValueNode(leftNum <= rightNum ? "1" : "0");
                    case TOKEN_AND:
                        return new ValueNode((leftNum != 0 && rightNum != 0) ? "1" : "0");
                    case TOKEN_OR:
                        return new ValueNode((leftNum != 0 || rightNum != 0) ? "1" : "0");
                    case TOKEN_XOR:
                        return new ValueNode(((leftNum != 0) != (rightNum != 0)) ? "1" : "0");
                    default:
                        return nullptr;
                }
                
                // Convert result back to string
                std::string resultStr = std::to_string(result);
                // Remove trailing zeros for cleaner output
                resultStr.erase(resultStr.find_last_not_of('0') + 1, std::string::npos);
                if (resultStr.back() == '.') {
                    resultStr.pop_back();
                }
                return new ValueNode(resultStr);
                
            } catch (const std::exception&) {
                // If parsing as numbers fails, try string concatenation for plus
                if (binOpNode->op == TOKEN_PLUS) {
                    return new ValueNode(leftVal->value + rightVal->value);
                }
                return nullptr;
            }
        }
    }
    
    return nullptr;
}

bool Preprocessor::isConstant(AST* node) {
    return dynamic_cast<ValueNode*>(node) != nullptr;
}