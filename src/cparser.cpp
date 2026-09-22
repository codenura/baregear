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

#include <cparser.h>
#include <definations.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

// Global pointer to store the AST context for retrieval
static clang::ASTContext* g_astContext = nullptr;
static clang::TranslationUnitDecl* g_translationUnit = nullptr;
static std::string g_lastError;

// Assembly instruction string to enum mapping
static std::map<std::string, AsmInstructionType> asmInstructionMap = {
    {"mov", ASM_MOV}, {"add", ASM_ADD}, {"sub", ASM_SUB}, {"mul", ASM_MUL},
    {"div", ASM_DIV}, {"imul", ASM_IMUL}, {"idiv", ASM_IDIV},
    {"and", ASM_AND}, {"or", ASM_OR}, {"xor", ASM_XOR}, {"not", ASM_NOT},
    {"shl", ASM_SHL}, {"shr", ASM_SHR}, {"sar", ASM_SAR},
    {"cmp", ASM_CMP}, {"test", ASM_TEST},
    {"jmp", ASM_JMP}, {"je", ASM_JE}, {"jne", ASM_JNE}, {"jg", ASM_JG},
    {"jl", ASM_JL}, {"jge", ASM_JGE}, {"jle", ASM_JLE}, {"ja", ASM_JA},
    {"jb", ASM_JB}, {"jae", ASM_JAE}, {"jbe", ASM_JBE}, {"jz", ASM_JZ},
    {"jnz", ASM_JNZ}, {"push", ASM_PUSH}, {"pop", ASM_POP},
    {"call", ASM_CALL}, {"ret", ASM_RET}, {"lea", ASM_LEA}, {"xchg", ASM_XCHG}
};

// Register list for detection
static std::set<std::string> asmRegisters = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
    "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
    "al", "bl", "cl", "dl", "ah", "bh", "ch", "dh"
};

class CParserASTConsumer : public clang::ASTConsumer {
public:
    void Initialize(clang::ASTContext& ctx) override {
        g_astContext = &ctx;
        g_translationUnit = ctx.getTranslationUnitDecl();
    }

    bool HandleTopLevelDecl(clang::DeclGroupRef DG) override {
        return true;
    }
};

class CParserFrontendAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, 
                                                           llvm::StringRef) override {
        return std::make_unique<CParserASTConsumer>();
    }

    void ExecuteAction() override {
        clang::ASTFrontendAction::ExecuteAction();
    }
};

class CParserDiagnosticConsumer : public clang::DiagnosticConsumer {
private:
    std::string errorMessages;

public:
    void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                         const clang::Diagnostic& Info) override {
        llvm::SmallString<100> DiagMsg;
        Info.FormatDiagnostic(DiagMsg);
        errorMessages += DiagMsg.str().str() + "\n";
    }

    std::string getErrors() const { return errorMessages; }
    void clear() { errorMessages.clear(); }
};

// Assembly parsing helper methods
std::string CParser::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool CParser::isRegister(const std::string& operand) {
    std::string lowerOp = toLower(operand);
    return asmRegisters.find(lowerOp) != asmRegisters.end();
}

bool CParser::isImmediate(const std::string& operand) {
    if (operand.empty()) return false;
    
    // Check for hex prefix
    if (operand.size() > 2 && operand[0] == '0' && (operand[1] == 'x' || operand[1] == 'X')) {
        return std::all_of(operand.begin() + 2, operand.end(),
                          [](unsigned char c) { return std::isxdigit(c); });
    }
    
    // Check for decimal
    return std::all_of(operand.begin(), operand.end(),
                      [](unsigned char c) { return std::isdigit(c) || c == '-'; });
}

bool CParser::isMemory(const std::string& operand) {
    return operand.find('[') != std::string::npos && operand.find(']') != std::string::npos;
}

bool CParser::isLabel(const std::string& operand) {
    // Labels typically start with a letter and contain letters, digits, underscores
    if (operand.empty() || !std::isalpha(operand[0])) return false;
    return std::all_of(operand.begin(), operand.end(),
                      [](unsigned char c) { return std::isalnum(c) || c == '_'; });
}

AsmOperandType CParser::determineOperandType(const std::string& operand) {
    if (isMemory(operand)) return ASM_OP_MEMORY;
    if (isRegister(operand)) return ASM_OP_REGISTER;
    if (isImmediate(operand)) return ASM_OP_IMMEDIATE;
    if (isLabel(operand)) return ASM_OP_LABEL;
    return ASM_OP_IMMEDIATE; // Default fallback
}

AsmOperand* CParser::parseOperand(const std::string& operand, int row, int col) {
    AsmOperandType type = determineOperandType(operand);
    return new AsmOperand(type, operand, row, col);
}

AsmInstructionType CParser::parseInstructionType(const std::string& instr) {
    std::string lowerInstr = toLower(instr);
    auto it = asmInstructionMap.find(lowerInstr);
    if (it != asmInstructionMap.end()) {
        return it->second;
    }
    return ASM_UNKNOWN;
}

AsmInstruction* CParser::parseInstruction(const std::string& line, int row, int col) {
    std::istringstream iss(line);
    std::string instructionStr;
    iss >> instructionStr;
    
    AsmInstructionType instrType = parseInstructionType(instructionStr);
    std::vector<AsmOperand*> operands;
    
    std::string operand;
    while (iss >> operand) {
        // Remove trailing comma
        if (!operand.empty() && operand.back() == ',') {
            operand.pop_back();
        }
        if (!operand.empty()) {
            operands.push_back(parseOperand(operand, row, col));
        }
    }
    
    return new AsmInstruction(instrType, operands, row, col);
}

AsmLabel* CParser::parseLabel(const std::string& line, int row, int col) {
    std::string labelName = line;
    // Remove trailing colon
    if (!labelName.empty() && labelName.back() == ':') {
        labelName.pop_back();
    }
    return new AsmLabel(labelName, row, col);
}

AsmDirective* CParser::parseDirective(const std::string& line, int row, int col) {
    std::istringstream iss(line);
    std::string directive;
    iss >> directive;
    
    std::string value;
    std::string token;
    while (iss >> token) {
        if (!value.empty()) value += " ";
        value += token;
    }
    
    return new AsmDirective(directive, value, row, col);
}

std::vector<AsmASTNode*> CParser::parseAssemblyCode(const std::string& asmCode) {
    std::vector<AsmASTNode*> ast;
    std::istringstream input(asmCode);
    std::string line;
    int rowNum = 1;
    
    while (std::getline(input, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || (line.size() >= 2 && line[0] == '/' && line[1] == '/')) {
            rowNum++;
            continue;
        }
        
        // Remove inline comments
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
            line.erase(line.find_last_not_of(" \t") + 1);
        }
        
        if (line.empty()) {
            rowNum++;
            continue;
        }
        
        // Check if it's a label
        if (line.back() == ':') {
            ast.push_back(parseLabel(line, rowNum, 1));
            rowNum++;
            continue;
        }
        
        // Check if it's a directive (starts with .)
        if (line[0] == '.') {
            ast.push_back(parseDirective(line, rowNum, 1));
            rowNum++;
            continue;
        }
        
        // Parse as instruction
        ast.push_back(parseInstruction(line, rowNum, 1));
        rowNum++;
    }
    
    return ast;
}

CParser::CParser(const std::string& code, CLanguageMode mode)
    : sourceCode(code), languageMode(mode), hasErrors(false) {
    // If assembly mode, parse assembly code immediately
    if (mode == CLanguageMode::ASM) {
        asmAST = parseAssemblyCode(code);
    }
}

CParser::~CParser() {
    // Clean up assembly AST
    for (auto node : asmAST) {
        delete node;
    }
    asmAST.clear();
}

bool CParser::parse() {
    hasErrors = false;
    g_lastError.clear();
    g_astContext = nullptr;
    g_translationUnit = nullptr;

    // Handle assembly mode - already parsed in constructor
    if (languageMode == CLanguageMode::ASM)
        return true; // Assembly AST is already built in constructor

    // Configure compiler arguments based on language mode
    std::vector<std::string> args;
    
    if (languageMode == CLanguageMode::C)
        args = {
            "cparser",
            "-std=c23",
            "-fsyntax-only",
            "-target", "x86_64-unknown-linux-gnu",
            "-I", "/usr/include"
        };
     else
        args = {
            "cparser",
            "-std=c++23",
            "-fsyntax-only",
            "-target", "x86_64-unknown-linux-gnu",
            "-I", "/usr/include"
        };

    // Create frontend action
    auto action = std::make_unique<CParserFrontendAction>();
    
    // Run the tool on the code
    bool success = clang::tooling::runToolOnCodeWithArgs(
        std::move(action),
        sourceCode,
        args
    );

    if (!success || g_astContext == nullptr) {
        hasErrors = true;
        if (g_lastError.empty())
            g_lastError = "Failed to parse C/C++ code - unknown error";

        bugDetected(g_lastError.c_str());
        return false;
    }

    // Store the captured AST
    astContext.reset(g_astContext);
    translationUnit.reset(g_translationUnit);
    
    return true;
}

clang::ASTContext* CParser::getASTContext() const {
    return astContext.get();
}

clang::TranslationUnitDecl* CParser::getTranslationUnit() const {
    return translationUnit.get();
}

bool CParser::hasParseErrors() const {
    return hasErrors;
}

std::string CParser::getLastError() const {
    return g_lastError;
}
