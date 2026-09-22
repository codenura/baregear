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

#include <debugger.h>
#include <runtime.h>
#include <definations.h>
#include <ai.h>
#include <dynvar.h>
#include <lldb/API/SBTarget.h>
#include <lldb/API/SBAddress.h>
#include <lldb/API/SBFunction.h>
#include <lldb/API/SBType.h>

extern "C" {
    #include <seccomp.h>
    #include <seccomp-syscalls.h>
    
    void handleSYSCALL(int notify_fd) {
        struct seccomp_notif *req = NULL;
        struct seccomp_notif_resp *resp = NULL;

        if (seccomp_notify_alloc(&req, &resp) < 0) {
            bugDetected("seccomp_notify_alloc failed");
            return;
        }

        while (seccomp_notify_receive(notify_fd, req) == 0) {
            if (seccomp_notify_id_valid(notify_fd, req->id) < 0)
                continue;

            int syscall_num = req->data.nr;
            __u32 arch = req->data.arch;
            __u64 instruction_pointer = req->data.instruction_pointer;
            __u64 *register_args = req->data.args;

            // Send payload to your compiler AI
            // Create vector from register arguments including rax (syscall number)
            __u64 all_regs[7]; // rax + 6 args
            all_regs[0] = syscall_num; // rax contains syscall number
            for (int i = 0; i < 6; i++)
                all_regs[i + 1] = register_args[i];

            vector registers;
            registers.address = (uintptr_t)all_regs;
            registers.sizePerBlks = sizeof(__u64);
            registers.count = 7; // rax + 6 arguments

            // vector result = guessTheBehavior(registers, syscall_num, arch);

            // Prepare response to resume or block the sandboxed process
            resp->id = req->id;
            resp->flags = 0;
            resp->error = 0; // Allow the syscall
            resp->val = 0;

            // Send the decision back to the kernel to unpause the tracee
            seccomp_notify_respond(notify_fd, resp);
        }

        seccomp_notify_free(req, resp);
    }

    

    void simLoop(int notify_fd) {
        struct seccomp_notif req;
        struct seccomp_notif_resp resp;

        while (1) {
            memset(&req, 0, sizeof(req));
            memset(&resp, 0, sizeof(resp));

            if (seccomp_notify_receive(notify_fd, &req) < 0) {
                perror("seccomp_notify_receive");
                break;
            }

            // Validate if the notification is still alive/valid (prevents race conditions)
            if (seccomp_notify_id_valid(notify_fd, req.id) < 0)
                continue;

            SymInfo symbolInfo = getLineNumber((uint64_t)req.data.instruction_pointer);
            addBreakpoint(symbolInfo.lineNumber + 1, &symbolInfo.sourcePath, (void*)recordVar);

            // Prepare the response packet
            resp.id = req.id;
            resp.flags = 0;
            resp.error = 0; // Allow the syscall
            resp.val = 0;

            // Send response back to unblock the guest
            if (seccomp_notify_respond(notify_fd, &resp) < 0) {
                // If the target process died or was interrupted, handle gracefully
                if (errno == ENOENT) continue;
                perror("seccomp_notify_respond");
                break;
            }
        }
    }
}

uintptr_t recordVar() {
    // Get function address and parameters from LLDB
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