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

#ifndef BAREGEAR_AI_H
#define BAREGEAR_AI_H
#include <dynvar.h>
#include <seccomp.h>

#ifdef __cplusplus
#include <cparser.h>

/* void fixBugByPossibility(clang::ASTContext *AST, clang::TranslationUnitDecl *tunit,
                        CLanguageMode lang, int section[], bhResult possibility); */

extern "C" {
#endif

// extern vector* guessTheBehavior(vector registers, int code, __u32 arch);

#ifdef __cplusplus
}
#endif

#endif //BAREGEAR_AI_H