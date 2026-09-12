/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 First Person
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

#ifndef CARSER_H
#define CARSER_H

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

enum class CLanguageMode {
    C,
    CPP,
    ASM
};

// Assembly AST node types (defined locally in cparser to avoid modifying parser.h)
enum AsmInstructionType {
    ASM_MOV, ASM_ADD, ASM_SUB, ASM_MUL, ASM_DIV, ASM_IMUL, ASM_IDIV,
    ASM_AND, ASM_OR, ASM_XOR, ASM_NOT, ASM_SHL, ASM_SHR, ASM_SAR,
    ASM_CMP, ASM_TEST, ASM_JMP, ASM_JE, ASM_JNE, ASM_JG, ASM_JL,
    ASM_JGE, ASM_JLE, ASM_JA, ASM_JB, ASM_JAE, ASM_JBE, ASM_JZ, ASM_JNZ,
    ASM_PUSH, ASM_POP, ASM_CALL, ASM_RET, ASM_LEA, ASM_XCHG, ASM_UNKNOWN
};

enum AsmOperandType {
    ASM_OP_REGISTER,
    ASM_OP_IMMEDIATE,
    ASM_OP_MEMORY,
    ASM_OP_LABEL
};

// Assembly AST base node
struct AsmASTNode {
    int row, col;
    AsmASTNode(int r = 0, int c = 0) : row(r), col(c) {}
    virtual ~AsmASTNode() = default;
};

// Assembly operand node
struct AsmOperand : AsmASTNode {
    AsmOperandType type;
    std::string value;
    AsmOperand(AsmOperandType t, std::string v, int r = 0, int c = 0)
        : AsmASTNode(r, c), type(t), value(std::move(v)) {}
};

// Assembly instruction node
struct AsmInstruction : AsmASTNode {
    AsmInstructionType instruction;
    std::vector<AsmOperand*> operands;
    AsmInstruction(AsmInstructionType instr, std::vector<AsmOperand*> ops, int r = 0, int c = 0)
        : AsmASTNode(r, c), instruction(instr), operands(std::move(ops)) {}
    ~AsmInstruction() override {
        for (auto op : operands) delete op;
    }
};

// Assembly label node
struct AsmLabel : AsmASTNode {
    std::string name;
    AsmLabel(std::string n, int r = 0, int c = 0) : AsmASTNode(r, c), name(std::move(n)) {}
};

// Assembly directive node (e.g., .section, .global)
struct AsmDirective : AsmASTNode {
    std::string directive;
    std::string value;
    AsmDirective(std::string dir, std::string v, int r = 0, int c = 0)
        : AsmASTNode(r, c), directive(std::move(dir)), value(std::move(v)) {}
};

class CParser {
private:
    std::string sourceCode;
    CLanguageMode languageMode;
    std::unique_ptr<clang::ASTContext> astContext;
    std::unique_ptr<clang::TranslationUnitDecl> translationUnit;
    bool hasErrors;

    // Assembly AST storage
    std::vector<AsmASTNode*> asmAST;

    // Assembly parsing helper methods
    static std::string toLower(const std::string& str);
    static bool isRegister(const std::string& operand);
    static bool isImmediate(const std::string& operand);
    static bool isMemory(const std::string& operand);
    static bool isLabel(const std::string& operand);
    static AsmOperandType determineOperandType(const std::string& operand);
    static AsmOperand* parseOperand(const std::string& operand, int row, int col);
    static AsmInstructionType parseInstructionType(const std::string& instr);
    static AsmInstruction* parseInstruction(const std::string& line, int row, int col);
    static AsmLabel* parseLabel(const std::string& line, int row, int col);
    static AsmDirective* parseDirective(const std::string& line, int row, int col);
    static std::vector<AsmASTNode*> parseAssemblyCode(const std::string& asmCode);

public:
    CParser(const std::string& code, CLanguageMode mode = CLanguageMode::CPP);
    ~CParser();

    bool parse();
    clang::ASTContext* getASTContext() const;
    clang::TranslationUnitDecl* getTranslationUnit() const;
    bool hasParseErrors() const;
    std::string getLastError() const;
    
    // Assembly AST access
    const std::vector<AsmASTNode*>& getAssemblyAST() const { return asmAST; }
};

#endif // CARSER_H
