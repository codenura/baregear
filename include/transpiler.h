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

#ifndef TRANSPILER_H
#define TRANSPILER_H
#include <iostream>
#include <sstream>
#include <vector>
#include <stdbool.h>
#include <dynvar.h>
#include <parser.h>

extern vector* condFlags;
typedef struct {
    int col;
    AST* condition;
} CondFlag;

class Transpiler {
private:
    int idx;
    std::vector<AST*> nodes;
    bool isVarDTYPEUsed = false;
    std::stringstream str, sstr, hstr;
    DATATYPE vdtype;
    vector* condFlagsMap;
    bool inMain;

    std::string factor(AST* body);
    std::string statement(AST* node);
    inline std::string getCDataType(DATATYPE &dtype);
    DATATYPE getOperandType(AST* node);
public:
    Transpiler(std::vector<AST*> n) : nodes(std::move(n)), condFlagsMap(nullptr), inMain(false) { }
    std::string transpile();
};

#endif // TRANSPILER_H