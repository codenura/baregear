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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>

#include <debugger.h>
#include <runtime.h>
#include <definations.h>
#include <ai.h>
#include <dynvar.h>
#include <emulatorUtil.h>
#include <unicorn/x86.h>
#include <lldb/API/SBTarget.h>
#include <lldb/API/SBAddress.h>
#include <lldb/API/SBFunction.h>
#include <lldb/API/SBType.h>

extern "C" {
    static void handleSYSCALL(uc_engine *engine, void *) {
        uint64_t instruction_pointer = 0;
        uint64_t flags = 0;
        const uint64_t syscall_result = 0;
        const uint64_t syscall_instruction_size = 2;

        if (uc_reg_read(engine, UC_X86_REG_RIP, &instruction_pointer) != UC_ERR_OK ||
            uc_reg_read(engine, UC_X86_REG_RFLAGS, &flags) != UC_ERR_OK) {
            uc_emu_stop(engine);
            return;
        }

        instruction_pointer += syscall_instruction_size;
        if (uc_reg_write(engine, UC_X86_REG_RCX, &instruction_pointer) != UC_ERR_OK ||
            uc_reg_write(engine, UC_X86_REG_R11, &flags) != UC_ERR_OK ||
            uc_reg_write(engine, UC_X86_REG_RAX, &syscall_result) != UC_ERR_OK ||
            uc_reg_write(engine, UC_X86_REG_RIP, &instruction_pointer) != UC_ERR_OK)
            uc_emu_stop(engine);
    }

    uc_err startEmulation(uc_engine **engine) {
        if (engine == nullptr)
            return UC_ERR_ARG;

        uc_err error = uc_open(UC_ARCH_X86, UC_MODE_64, engine);
        if (error != UC_ERR_OK)
            return error;

        uc_hook syscall_hook;
        error = uc_hook_add(*engine, &syscall_hook, UC_HOOK_INSN,
                            reinterpret_cast<void *>(handleUnicornSyscall), nullptr,
                            1, 0, UC_X86_INS_SYSCALL);
        if (error != UC_ERR_OK) {
            uc_close(*engine);
            *engine = nullptr;
        }

        return error;
    }
}

uintptr_t recordVar() {
    uintptr_t function = 0;
    vector* params = nullptr;

    if (!g_process.IsValid()) {
        bugDetected("No active debugging process.");
        return 0;
    }

    lldb::SBThread thread = g_process.GetSelectedThread();
    if (!thread.IsValid()) {
        bugDetected("No valid thread found.");
        return 0;
    }
    
    lldb::SBFrame frame = thread.GetSelectedFrame();
    if (!frame.IsValid()) {
        bugDetected("No valid frame found.");
        return 0;
    }
    
    // Get function address from current frame
    lldb::SBAddress addr = frame.GetPCAddress();
    if (addr.IsValid())
        function = (uintptr_t)addr.GetLoadAddress(g_target);
    
    // Get function parameters from current frame
    lldb::SBValueList variables = frame.GetVariables(true, true, false, true);
    size_t numVariables = variables.GetSize();
    
    // Create a vector to hold parameter values as VariableInfo
    static VariableInfo param_values[16]; // Max 16 parameters
    static vector param_vector;
    param_vector.address = (uintptr_t)param_values;
    param_vector.sizePerBlks = sizeof(VariableInfo);
    param_vector.count = 0;
    
    for (size_t i = 0; i < numVariables && i < 16; i++) {
        lldb::SBValue variable = variables.GetValueAtIndex(i);
        if (variable.IsValid()) {
            const std::string name = variable.GetName();
            // Check if this is a function argument (not a local variable)
            if (!name.empty()) {
                VariableInfo paramInfo;
                paramInfo.variableName = strdup(name.c_str());
                paramInfo.value.address = 0;
                paramInfo.value.length = 0;
                paramInfo.pid = g_process.GetProcessID();
                
                const std::string valueStr = variable.GetValue();
                if (!valueStr.empty())
                    setValue(&paramInfo.value, (char*)valueStr.c_str());
                
                param_values[param_vector.count++] = paramInfo;
            }
        }
    }
    
    if (param_vector.count > 0)
        params = &param_vector;
    
    VRecord record;
    record.function = function;
    record.params = params;
    if (addr.IsValid()) {
        // Retrieve the SBFunction container from the address
        lldb::SBFunction lldb_function = addr.GetFunction();
        if (lldb_function.IsValid()) {
            lldb::SBType func_type = lldb_function.GetType();
            if (func_type.IsValid()) {
                lldb::SBType return_type = func_type.GetFunctionReturnType();
                if (return_type.IsValid() && strcmp(return_type.GetName(), "void") != 0) {
                    VariableInfo retValue = getReturnValue();
                    lgr valueBuffer;
                    getValue(retValue.value, &valueBuffer);
                    if (valueBuffer[0])
                        setValue(&record.value, valueBuffer);
                }

                lldb::SBTypeList arg_types = func_type.GetFunctionArgumentTypes();
                const size_t pcount = arg_types.GetSize();

                for (size_t i = 0; i < pcount; i++) {
                    lldb::SBType param_type = arg_types.GetTypeAtIndex(i);
                    param_type.GetName();
                }
            } else
                bugDetected("Function type signature unavailable (stripped symbols?).");
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "No debug/symbol function found at address 0x%lx", function);
            bugDetected(buf);
        }
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "Failed to resolve address: 0x%lx", function);
        bugDetected(buf);
    }
    
    lgr recordBuffer;
    memcpy(recordBuffer, &record, sizeof(record));
    vectorAppend(VRecords, recordBuffer);
    return function;
}