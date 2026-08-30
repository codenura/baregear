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

#ifndef DYNVAR_H
#define DYNVAR_H

#include <stdint.h>

#define VECTOR_FORMULA(var, idx)    ((void *)((char *)(var)->address + ((idx) * (var)->sizePerBlks)))

typedef struct {
    uintptr_t       address;
    unsigned int    length;
} dynvar;

typedef struct {
    uintptr_t       address;
    unsigned int    sizePerBlks;
    unsigned int    count;
} vector;

typedef enum {
    SUCCESS,
    ILVAR,
    BFROVRFLW
} DYNVAR_CODE;

#ifdef __cplusplus
extern "C" {
#endif

extern void vectorInit(vector* var, unsigned int length);
extern DYNVAR_CODE vectorAppend(vector* var, void* source);
extern long vectorGetValue(vector* var, int index);
extern int vectorFind(vector* var, long value);
extern DYNVAR_CODE vectorDelete(vector* var, int index);
extern void vectorDeleteAll(vector* var);

extern void setValue(dynvar* var, char* value);
extern long* getValue(dynvar var);

#ifdef __cplusplus
}
#endif

#endif // DYNVAR_H