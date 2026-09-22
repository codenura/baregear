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

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <vector>
#include <variant>
#include <map>
#include <lexer.h>
#include <vector>

enum CLASS_NODE_TYPE {
    PUBLIC,
    PROTECTED,
    PRIVATE
};

enum DATATYPE {
    INT,
    FLOAT,
    DOUBLE,
    SHORT,
    LONG,
    NUMBER,
    STRING,
    LIST,
    VARIANT,
    BOOL
};

struct AST {
    int row, col;
    AST() : row(0), col(0) {}
    AST(int r, int c) : row(r), col(c) {}
    virtual ~AST() = default;
};

struct ValueNode : AST {
    std::string value;
    ValueNode(std::string v, int r = 1, int c = 1) : AST(r, c), value(std::move(v)) {}
};

struct BinOpNode : AST {
    AST* left;
    TOKEN_TYPE op;
    AST* right;

    BinOpNode(AST* l, TOKEN_TYPE o, AST* r, int row = 0, int col = 0) : AST(row, col), left(std::move(l)), op(std::move(o)),
                                              right(std::move(r)) { }
};

struct VariableNode : AST {
    std::string name;
    DATATYPE datatype;
    bool isDecl;

    VariableNode(std::string n, DATATYPE d, bool decl = false, int r = 1, int c = 1) : AST(r, c), name(std::move(n)), datatype(std::move(d)), isDecl(decl) { }
};

struct CallNode : VariableNode {
    std::vector<AST*> param;
    CallNode(std::string n, std::vector<AST*> p, int r = 1, int c = 1) :
                                                           VariableNode(std::move(n), VARIANT, false, r, c),
                                                           param(std::move(p)) { }
};

struct FunctionNode : CallNode {
    std::vector<AST*> body;
    FunctionNode(std::string n, std::vector<VariableNode*> p, std::vector<AST*> b, int r = 1, int c = 1) :
                                                        CallNode(std::move(n), std::vector<AST*>(p.begin(), p.end()), r, c),
                                                        body(std::move(b)) { }
};

struct AssignNode : AST {
    std::string name;
    AST* node;

    AssignNode(std::string n, AST* v, int r = 1, int c = 1) : AST(r, c), name(std::move(n)), node(std::move(v)) { }
};

struct ConditionNode : BinOpNode {
    ConditionNode(AST* l, TOKEN_TYPE o, AST* r, int row = 0, int col = 0) : BinOpNode(std::move(l), o, std::move(r), row, col) { }
};

struct IfWhileNode : AST {
    AST* condition;
    TOKEN_TYPE sign;
    std::vector<AST*> body;
    char prefix{};

    IfWhileNode(AST* c, std::vector<AST*> b, TOKEN_TYPE s, int r = 1, int col = 0) :
                                                AST(r, col),
                                                condition(std::move(c)),
                                                body(std::move(b)), sign(std::move(s)) { }
    IfWhileNode(AST* c, std::vector<AST*> b, TOKEN_TYPE s, char p, int r = 1, int col = 0) : AST(r, col), body(std::move(b)),
                  condition(std::move(c)), sign(std::move(s)), prefix(std::move(p)) { }
};

struct CaseNode : AST {
    AST* value;
    bool isDefault;
    std::vector<AST*> body;
    CaseNode(AST* v, bool def, std::vector<AST*> b, int r = 1, int c = 1) : AST(r, c), value(std::move(v)), isDefault(def), body(std::move(b)) {}
};

struct SwitchNode : AST {
    AST* condition;
    std::vector<AST*> cases;
    SwitchNode(AST* c, std::vector<AST*> cs, int r = 1, int col = 0) : AST(r, col), condition(std::move(c)), cases(std::move(cs)) {}
};

struct DefineNode : AST {
    std::string name;
    std::string value;

    DefineNode(std::string n, std::string v, int r = 1, int c = 1) : AST(r, c), name(std::move(n)), value(std::move(v)) { }
};

struct FeatureNode : AST {
    std::string featureName;
    bool enabled;

    FeatureNode(std::string name, bool enable, int r = 1, int c = 1) : AST(r, c), featureName(std::move(name)), enabled(enable) { }
};

struct InlineCodeNode : AST {
    std::string code;
    TOKEN_TYPE language;

    InlineCodeNode(std::string c, TOKEN_TYPE lang, int r = 1, int col = 0) : AST(r, col), code(std::move(c)), language(std::move(lang)) { }
};

struct EndNode : AST {
    TOKEN_TYPE endType;

    EndNode(TOKEN_TYPE type, int r = 1, int c = 1) : AST(r, c), endType(std::move(type)) { }
};

struct StructNode : AST {
    std::vector<ValueNode> structure;
    StructNode(std::vector<ValueNode> Struct, int r = 1, int c = 1) : AST(r, c), structure(std::move(Struct)) { }
};

struct ClassNode : AST {
    std::map<ValueNode, CLASS_NODE_TYPE> classStructure;
    ClassNode(std::map<ValueNode, CLASS_NODE_TYPE> ClassStruct, int r = 1, int c = 1) :
                             AST(r, c), classStructure(std::move(ClassStruct)) { }
};

struct ReturnNode : AST {
    AST* node;
    ReturnNode(AST* n, int r = 1, int c = 1) : AST(r, c), node(std::move(n)) { }
};

struct BooleanNode : AST {
    bool condition;
    BooleanNode(bool c, int r = 1, int col = 1) : AST(r, col), condition(std::move(c)) { }
};

typedef enum {
    IMPORTANCE_URGENT,
    IMPORTANCE_HIGH,
    IMPORTANCE_MEDIUM,
    IMPORTANCE_LOW
} STMTImportance;

struct ImportanceNode : AST {
    STMTImportance level;
    std::vector<AST*> body;

    ImportanceNode(std::vector<AST*> b, STMTImportance l, int r = 1, int c = 1) :
                                                    body(std::move(b)),
                                                    level(std::move(l)) {}
};

struct ConditionalFlagNode : AST {
    AST* condition;
    std::vector<AST*> body;
    int row, col;
    ConditionalFlagNode(AST* cond, std::vector<AST*> b, int r = 1, int c = 1) : condition(cond), body(b), row(r), col(c) {}
};

struct NoChangeNode : AST {
    std::vector<AST*> body;
    int row, col;
    NoChangeNode(std::vector<AST*> b, int r = 1, int c = 1) : body(b), row(r), col(c) {}
};

class Parser {
private:
    std::vector<TOKEN> tokens;
    std::vector<Lexer> lexs;
    unsigned int idx = 0;

    AST* term();
    AST* expr();
    AST* factor();
    std::vector<AST*> statement();
    std::vector<AST*> parseBody(unsigned int baseCol);
    bool isFunctionDefinition();
    inline bool isAtEnd() {
        return idx >= tokens.size();
    }
public:
    Parser(std::vector<TOKEN> t) : tokens(std::move(t)) { }
    std::vector<AST*> parse();
};

#endif // PARSER_H
