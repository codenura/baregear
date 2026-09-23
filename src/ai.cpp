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
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <map>
#include <memory>
#include <capstone/capstone.h>
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <cparser.h>
#include <ai.h>
#include <dynvar.h>
#include <analyzer.h>
#include <definations.h>

extern csh cpstHandle;
/* #define appendResult(result) do { \
    src = result; \
    vectorAppend(possibilities, (void*)(long)src); \
} while(0) */

class ParentMapBuilder : public clang::RecursiveASTVisitor<ParentMapBuilder> {
public:
    std::map<clang::Stmt*, clang::Stmt*> parentMap;
    std::map<clang::Decl*, clang::Stmt*> declParentMap;
    std::vector<clang::Stmt*> stack;

    bool TraverseStmt(clang::Stmt *S) {
        if (!S) return true;
        if (!stack.empty())
            parentMap[S] = stack.back();
        stack.push_back(S);
        bool result = clang::RecursiveASTVisitor<ParentMapBuilder>::TraverseStmt(S);
        stack.pop_back();
        return result;
    }

    bool TraverseDecl(clang::Decl *D) {
        if (!D) return true;
        if (!stack.empty())
            declParentMap[D] = stack.back();
        return clang::RecursiveASTVisitor<ParentMapBuilder>::TraverseDecl(D);
    }
};

class SectionASTExtractor : public clang::RecursiveASTVisitor<SectionASTExtractor> {
private:
    clang::SourceManager &SM;
    int startLine;
    int endLine;

public:
    std::vector<clang::Decl*> ExtractedDecls;

    SectionASTExtractor(clang::SourceManager &sm, int start, int end)
        : SM(sm), startLine(start), endLine(end) {}

    // Hook into declaration visits
    bool VisitDecl(clang::Decl *D) {
        if (!D || D->getLocation().isInvalid()) return true;

        clang::FullSourceLoc beginLoc = clang::FullSourceLoc(D->getBeginLoc(), SM);

        if (beginLoc.isInSystemHeader()) return true;

        unsigned nodeStartLine = beginLoc.getExpansionLineNumber();

        if (nodeStartLine >= startLine && nodeStartLine <= endLine)
            ExtractedDecls.push_back(D);
        return true;
    }
};

extern "C" {
    /* vector* guessTheBehavior(vector registers, int code, uint32_t arch) {
        const long rax = vectorGetValue(&registers, 0); // Syscall number (rax)
        const long rdi = vectorGetValue(&registers, 1); // First argument (rdi on x86_64)
        const long rsi = vectorGetValue(&registers, 2); // Second argument (rsi on x86_64)
        const long rdx = vectorGetValue(&registers, 3); // Third argument (rdx on x86_64)
        const long r10 = vectorGetValue(&registers, 4); // Fourth argument (r10 on x86_64)
        const long r8 = vectorGetValue(&registers, 5);  // Fifth argument (r8 on x86_64)
        const long r9 = vectorGetValue(&registers, 6);  // Sixth argument (r9 on x86_64)

        // Allocate and initialize the possibilities vector
        vector* possibilities = (vector*)malloc(sizeof(vector));
        if (!possibilities) return NULL;
        possibilities->address = 0;
        possibilities->sizePerBlks = sizeof(bhResult);
        possibilities->count = 0;

        bhResult src;

        if (rax == 0) { // read syscall
            appendResult(IO_OPERATION_FAILED);
            appendResult(EOF_REACHED_UNEXPECTEDLY);

            if (rdi < 0)
                appendResult(PERMISSION_DENIED);
        }
        else if (rax == 1) { // write syscall
            appendResult(IO_OPERATION_FAILED);
            appendResult(DISK_QUOTA_EXCEEDED);

            if (rdi < 0)
                appendResult(PERMISSION_DENIED);
        }
        else if (rax == 2) { // open syscall
            src = FILE_NOT_FOUND;
            vectorAppend(possibilities, (void*)(long)src);

            if (rdi == 0 || rdi == -1)
                return possibilities;

            appendResult(FILE_ACCESS_DENIED);
            appendResult(EOF_REACHED_UNEXPECTEDLY);
            appendResult(IO_OPERATION_FAILED);
        }
        else if (rax == 3) { // close syscall
            appendResult(IO_OPERATION_FAILED);

            if (rdi < 0)
                appendResult(PERMISSION_DENIED);
        }
        else if (rax == 9) { // mmap syscall
            appendResult(MEMORY_LEAK);
            appendResult(OUT_OF_BOUNDS_READ);
            appendResult(OUT_OF_BOUNDS_WRITE);

            if (rdi == 0 && rsi == 0)
                appendResult(NULL_POINTER_DEREFERENCE);
        }
        else if (rax == 11) { // munmap syscall
            appendResult(INVALID_FREE);
            appendResult(HEAP_CORRUPTION);
        }
        else if (rax == 21) { // access syscall
            src = FILE_NOT_FOUND;
            vectorAppend(possibilities, (void*)(long)src);

            if (rdi == 0 || rdi == -1)
                return possibilities;

            appendResult(PERMISSION_DENIED);
            appendResult(FILE_ACCESS_DENIED);
        }
        else if (rax == 33) { // dup2 syscall
            appendResult(IO_OPERATION_FAILED);

            if (rdi < 0 || rsi < 0)
                appendResult(PERMISSION_DENIED);
        }
        else if (rax == 41) { // socket syscall
            appendResult(UNAUTHORIZED_NETWORK_ACCESS);
            appendResult(RESOURCE_LIMIT_EXCEEDED);
        }
        else if (rax == 42) { // connect syscall
            appendResult(UNAUTHORIZED_NETWORK_ACCESS);
            appendResult(PERMISSION_DENIED);

            if (rdi < 0)
                appendResult(IO_OPERATION_FAILED);
        }
        else if (rax == 56 || rax == 57 || rax == 58) { // clone/fork/vfork
            appendResult(THREAD_CREATION_FAILED);
            appendResult(RESOURCE_STARVATION);
        }
        else if (rax == 202) { // futex syscall
            appendResult(DEADLOCK);
            appendResult(RESOURCE_STARVATION);
            appendResult(TIMEOUT);
        }
        else
            appendResult(UNKNOWN_ERROR);
        return possibilities;
    } */
}

static bool isIsOpenCheck(clang::Stmt *S, const std::string &varName) {
    if (!S) return false;

    if (auto *unaryOp = clang::dyn_cast<clang::UnaryOperator>(S))
        if (unaryOp->getOpcode() == clang::UO_LNot || unaryOp->getOpcode() == clang::UO_Not)
            return isIsOpenCheck(unaryOp->getSubExpr(), varName);

    if (auto *callExpr = clang::dyn_cast<clang::CallExpr>(S))
        if (auto *membExpr = clang::dyn_cast<clang::MemberExpr>(callExpr->getCallee()))
            if (membExpr->getMemberNameInfo().getAsString() == "is_open")
                if (auto *dr = clang::dyn_cast<clang::DeclRefExpr>(membExpr->getBase()))
                    return dr->getNameInfo().getAsString() == varName;

    return false;
}

std::string transpileASTToCode(clang::ASTContext &Context, clang::Stmt *astNode) {
    if (!astNode) return "";

    std::string sourceCodeBuffer;
    llvm::raw_string_ostream outputStream(sourceCodeBuffer);
    clang::PrintingPolicy policy(Context.getLangOpts());
    policy.Indentation = 4;
    policy.Bool = true;
    astNode->printPretty(outputStream, nullptr, policy);
    outputStream.flush();

    return sourceCodeBuffer;
}

/*
 * Agent: Codex
 * LLM: GPT-5.6 Luna
 */
class FunctionNodeFinder : public clang::RecursiveASTVisitor<FunctionNodeFinder> {
public:
    explicit FunctionNodeFinder(const std::string &name) : functionName(name) {}

    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl) {
        if (!functionDecl || !functionDecl->isThisDeclarationADefinition())
            return true;

        if (functionDecl->getNameAsString() == functionName) {
            functionNode = functionDecl;
            return false;
        }

        return true;
    }

    clang::FunctionDecl *getFunctionNode() const { return functionNode; }

private:
    const std::string &functionName;
    clang::FunctionDecl *functionNode = nullptr;
};

// Lowers calls in a Clang function body to the assembly representation used by
// CParser. The generated nodes own their operands through AsmInstruction.
// Universal AST converter for lowering Clang call nodes and inline assembly statements
// to the assembly AST representation used by CParser.
class AssemblyASTConverter
    : public clang::RecursiveASTVisitor<AssemblyASTConverter> {
public:
    explicit AssemblyASTConverter(clang::SourceManager &sourceManager)
        : sourceManager(sourceManager) {}

    // 1. Visit Call Expressions (clang::CallExpr)
    bool VisitCallExpr(clang::CallExpr *callNode) {
        if (!callNode)
            return true;

        const clang::FunctionDecl *callee = callNode->getDirectCallee();
        if (!callee) {
            bugDetected("Cannot convert an indirect function call to an assembly AST.");
            conversionFailed = true;
            return false;
        }

        const clang::PresumedLoc location =
            sourceManager.getPresumedLoc(callNode->getExprLoc());
        const int row = location.isValid() ? static_cast<int>(location.getLine()) : 0;
        const int column = location.isValid() ? static_cast<int>(location.getColumn()) : 0;

        std::vector<AsmOperand *> operands;
        operands.push_back(new AsmOperand(
            ASM_OP_LABEL, callee->getNameAsString(), row, column));
        assemblyAST.push_back(std::make_unique<AsmInstruction>(
            ASM_CALL, std::move(operands), row, column));
        return true;
    }

    // 2. Visit GCC Inline Assembly Statements (clang::GCCAsmStmt)
    bool VisitGCCAsmStmt(clang::GCCAsmStmt *asmNode) {
        if (!asmNode)
            return true;

        const clang::PresumedLoc location =
            sourceManager.getPresumedLoc(asmNode->getAsmLoc());
        const int row = location.isValid() ? static_cast<int>(location.getLine()) : 0;
        const int column = location.isValid() ? static_cast<int>(location.getColumn()) : 0;

        std::string asmString = asmNode->getAsmString()->getString().str();

        std::vector<AsmOperand *> operands;
        operands.push_back(new AsmOperand(ASM_OP_LABEL, asmString, row, column));
        
        assemblyAST.push_back(std::make_unique<AsmInstruction>(
            ASM_INLINE_ASM, std::move(operands), row, column));
        return true;
    }

    // 3. Visit MS Inline Assembly Statements (clang::MSAsmStmt)
    bool VisitMSAsmStmt(clang::MSAsmStmt *asmNode) {
        if (!asmNode)
            return true;

        const clang::PresumedLoc location =
            sourceManager.getPresumedLoc(asmNode->getAsmLoc());
        const int row = location.isValid() ? static_cast<int>(location.getLine()) : 0;
        const int column = location.isValid() ? static_cast<int>(location.getColumn()) : 0;

        std::string asmString = asmNode->getAsmString().str();

        std::vector<AsmOperand *> operands;
        operands.push_back(new AsmOperand(ASM_OP_LABEL, asmString, row, column));

        assemblyAST.push_back(std::make_unique<AsmInstruction>(
            ASM_INLINE_ASM, std::move(operands), row, column));
        return true;
    }

    const std::vector<std::unique_ptr<AsmASTNode>> &getAssemblyAST() const {
        return assemblyAST;
    }

    bool hasConversionFailed() const { return conversionFailed; }

private:
    clang::SourceManager &sourceManager;
    std::vector<std::unique_ptr<AsmASTNode>> assemblyAST;
    bool conversionFailed = false;
};

void fixVuln(clang::ASTContext &Context, clang::Stmt *astNode, dynvar functionName) {
    /*
     * Agent: Codex
     * LLM: GPT-5.6 Luna
     */
    
    if (!astNode) {
        bugDetected("Source AST node is empty.");
        return;
    }

    if (functionName.address == (uintptr_t)0 || functionName.length == 0) {
        bugDetected("Function name is empty.");
        return;
    }

    const std::string requestedFunctionName(
        static_cast<const char *>(reinterpret_cast<const void *>(functionName.address)),
        functionName.length);
    FunctionNodeFinder finder(requestedFunctionName);
    finder.TraverseDecl(Context.getTranslationUnitDecl());

    clang::FunctionDecl *functionNode = finder.getFunctionNode();
    if (!functionNode) {
        bugDetected("Requested function was not found in the source AST.");
        return;
    }

    if (!functionNode->getBody()) {
        bugDetected("Requested function has no body to convert.");
        return;
    }

    vector results;

    for (clang::Stmt *child : functionNode->getBody()->children()) {
        if (!child)
            continue;

        if (clang::isa<clang::CallExpr>(child) || clang::isa<clang::GCCAsmStmt>(child)
            || clang::isa<clang::MSAsmStmt>(child)) {
            AssemblyASTConverter converter(Context.getSourceManager());
            converter.TraverseStmt(functionNode->getBody());
            if (converter.hasConversionFailed())
                return;

            AssemblyASTConverter converter(Context.getSourceManager());
            converter.TraverseStmt(functionNode->getBody());

            if (converter.hasConversionFailed()) {
                bugDetected("Assembly AST conversion failed.");
                return;
            }

            // Execute Assembly-level analysis
            vector results = analyzeCode(converter.getAssemblyAST());
        }
    }

    // Will Be Implemented
}

/* void fixBugByPossibility(clang::ASTContext* AST, clang::TranslationUnitDecl* tunit,
                         int section[], bhResult possibility) {
    if (!section) return;
    
    if (!AST || !tunit) return;
    
    clang::SourceManager &SM = AST->getSourceManager();
    SectionASTExtractor extractor(SM, section[0], section[1]);
    extractor.TraverseDecl(tunit);

    ParentMapBuilder pmb;
    pmb.TraverseDecl(tunit);

    clang::Rewriter editor(SM, AST->getLangOpts());

    if (possibility == FILE_ACCESS_DENIED || possibility == EOF_REACHED_UNEXPECTEDLY ||
        possibility == IO_OPERATION_FAILED || possibility == FILE_NOT_FOUND ||
        possibility == DISK_QUOTA_EXCEEDED || possibility == UNAUTHORIZED_FILE_ACCESS) {
        for (clang::Decl *node : extractor.ExtractedDecls) {
            if (auto varDecl = clang::dyn_cast<clang::VarDecl>(node)) {
                std::string typeName = varDecl->getType().getAsString();
                if (typeName.find("ifstream") != std::string::npos || 
                    typeName.find("fstream") != std::string::npos ||
                    typeName.find("ofstream") != std::string::npos) {
                    if (varDecl->hasInit()) {
                        if (auto *ctor = clang::dyn_cast<clang::CXXConstructExpr>(varDecl->getInit())) {
                            if (ctor->getNumArgs() > 0) {
                                std::string varName = varDecl->getNameAsString();
                                clang::SourceLocation endLoc = varDecl->getEndLoc();

                                clang::CompoundStmt *parentCompound = nullptr;
                                auto declIt = pmb.declParentMap.find(varDecl);
                                if (declIt != pmb.declParentMap.end()) {
                                    auto stmtIt = pmb.parentMap.find(declIt->second);
                                    if (stmtIt != pmb.parentMap.end())
                                        parentCompound = clang::dyn_cast<clang::CompoundStmt>(stmtIt->second);
                                }

                                bool hasIsOpenCheck = false;
                                if (parentCompound) {
                                    bool foundDecl = false;
                                    for (auto *child : parentCompound->children()) {
                                        if (foundDecl) {
                                            hasIsOpenCheck = isIsOpenCheck(child, varName);
                                            break;
                                        }
                                        if (auto *ds = clang::dyn_cast<clang::DeclStmt>(child))
                                            for (auto *d : ds->decls())
                                                if (d == varDecl) {
                                                    foundDecl = true;
                                                    break;
                                                }
                                    }
                                }

                                if (!hasIsOpenCheck)
                                    editor.InsertTextAfterToken(endLoc,
                                        "\nif (!" + varName + ".is_open())\n"
                                        "    std::cerr << \"Can't Open File\" << std::endl;");
                            }
                        }
                    }
                }
            }
        }
    } else if (possibility == OUT_OF_BOUNDS_READ || possibility == OUT_OF_BOUNDS_WRITE ||
               possibility == NULL_POINTER_DEREFERENCE) {
        for (clang::Decl *node : extractor.ExtractedDecls) {
            if (auto *varDecl = clang::dyn_cast<clang::VarDecl>(node)) {
                if (varDecl->hasInit()) {
                    clang::Expr* init = varDecl->getInit();
                    std::string varName = varDecl->getNameAsString();
                    clang::SourceLocation endLoc = varDecl->getEndLoc();

                    // Check for CXXConstructExpr (constructor calls)
                    if (auto ctorExpr = clang::dyn_cast<clang::CXXConstructExpr>(init)) {
                        clang::CXXConstructorDecl* ctorDecl = ctorExpr->getConstructor();
                        if (ctorDecl) {
                            std::string className = ctorDecl->getParent()->getNameAsString();
                            
                            // Check for vector/array constructors that might cause bounds issues
                            if (className.find("vector") != std::string::npos || 
                                className.find("array") != std::string::npos) {
                                if (possibility == OUT_OF_BOUNDS_READ || possibility == OUT_OF_BOUNDS_WRITE) {
                                    editor.InsertTextAfterToken(endLoc,
                                        "\n// Note: Check bounds when accessing " + varName);
                                }
                            }
                        }
                        
                        // Get constructor arguments for further processing
                        if (ctorExpr->getNumArgs() > 0) {
                            clang::Expr* arg = ctorExpr->getArg(0);
                            // Process constructor arguments if needed
                        }
                    }
                    
                    // Check for CallExpr (function calls like malloc, calloc, etc.)
                    if (auto callExpr = clang::dyn_cast<clang::CallExpr>(init)) {
                        if (auto callee = callExpr->getCallee()) {
                            if (auto declRef = clang::dyn_cast<clang::DeclRefExpr>(callee)) {
                                if (auto funcDecl = clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                                    std::string funcName = funcDecl->getNameAsString();
                                    
                                    // Check for memory allocation functions
                                    if (funcName == "malloc" || funcName == "calloc" || 
                                        funcName == "realloc" || funcName == "alloca") {
                                        if (possibility == NULL_POINTER_DEREFERENCE)
                                            editor.InsertTextAfterToken(endLoc,
                                                "\nif (" + varName + " == nullptr)\n"
                                                "    std::cerr << \"Allocation Failed\" << std::endl;");
                                        if (possibility == OUT_OF_BOUNDS_READ || possibility == OUT_OF_BOUNDS_WRITE)
                                            editor.InsertTextAfterToken(endLoc,
                                                "\n// Note: Ensure allocation size is sufficient for " + varName);
                                    }
                                }
                            }
                        }
                    }
                    
                    // Check for CastExpr (like void* x = (void*)malloc(512))
                    if (auto castExpr = clang::dyn_cast<clang::CastExpr>(init)) {
                        clang::Expr* subExpr = castExpr->getSubExpr();
                        if (auto callExpr = clang::dyn_cast<clang::CallExpr>(subExpr)) {
                            if (auto callee = callExpr->getCallee()) {
                                if (auto declRef = clang::dyn_cast<clang::DeclRefExpr>(callee)) {
                                    if (auto funcDecl = clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                                        std::string funcName = funcDecl->getNameAsString();
                                        
                                        if (funcName == "malloc" || funcName == "calloc") {
                                            if (possibility == NULL_POINTER_DEREFERENCE) {
                                                editor.InsertTextAfterToken(endLoc,
                                                    "\nif (" + varName + " == nullptr)\n"
                                                    "    std::cerr << \"Null pointer allocation\" << std::endl;");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
} */
