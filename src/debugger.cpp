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
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>

#include <lldb/API/SBDebugger.h>
#include <lldb/API/SBTarget.h>
#include <lldb/API/SBProcess.h>
#include <lldb/API/SBListener.h>
#include <lldb/API/SBBreakpoint.h>
#include <lldb/API/SBBreakpointLocation.h>
#include <lldb/API/SBThread.h>
#include <lldb/API/SBFrame.h>
#include <lldb/API/SBValue.h>

#include <debugger.h>
#include <definations.h>
#include <dynvar.h>
#include <preprocessor.h>
#include <transpiler.h>

static lldb::SBDebugger g_debugger;
lldb::SBTarget g_target;
lldb::SBProcess g_process;
static bool g_debuggerInitialized = false;
typedef void (*BreakpointCallback)(int line, const std::string sourceFile);
static std::map<lldb::break_id_t, void*> g_breakpointCallbacks;
int cfidx = 0;

extern "C" {
    // vector* VMaps;
    vector* VRecords;

    void startDebugger(dynvar* host, unsigned short port, const void* buffer, size_t bufrSize) {
        if (g_debuggerInitialized)
            return;

        g_debugger = lldb::SBDebugger::Create();
        if (!g_debugger.IsValid()) {
            bugDetected("Failed to create LLDB debugger.");
            return;
        }

        g_debugger.SetAsync(false);

        char connectionUrl[128];
        snprintf(connectionUrl, sizeof(connectionUrl), "connect://%s:%u", (char*)getValue(*host), port);

        lldb::SBError error;
        lldb::SBTarget target = g_debugger.CreateTarget("", "", "", true, error);

        if (!target.IsValid()) {
            bugDetected("Failed to create LLDB target.");
            lldb::SBDebugger::Destroy(g_debugger);
            return;
        }

        g_target = target;

        lldb::SBListener listener = g_debugger.GetListener();
        lldb::SBProcess process = g_target.ConnectRemote(listener, connectionUrl, nullptr, error);

        if (!process.IsValid() || error.Fail()) {
            bugDetected("Failed to connect to remote debugger.");
            lldb::SBDebugger::Destroy(g_debugger);
            return;
        }

        g_process = process;

        if (buffer && bufrSize > 0) {
    #ifdef __linux__
            int mem_fd = memfd_create("ram_symbol_module", MFD_CLOEXEC);
            if (mem_fd != -1) {
                ssize_t written = write(mem_fd, buffer, bufrSize);
                if (written > 0) {
                    char path_buf[64];
                    snprintf(path_buf, sizeof(path_buf), "/proc/self/fd/%d", mem_fd);

                    lldb::SBModule module = g_target.AddModule(path_buf, "x86_64-pc-linux", nullptr);
                    if (module.IsValid())
                        bugDetected("Failed to load symbols from memory buffer.");
                } else
                    bugDetected("Failed to write symbol data to memory fd.");
                close(mem_fd);
            } else
                bugDetected("Failed to create memory file descriptor for symbols.");
    #endif
        }

        g_debuggerInitialized = true;
    }

    void checkCond() {
        CondFlag cflag = (CondFlag)vectorGetValue(condFlags, cfidx);
        if (auto BinOp = dynamic_cast<BinOpNode*>(cflag.condition)) {
            dynvar valL, valR;
        }
        cfidx++;
    }

    void stopDebugger() {
        if (g_process.IsValid()) {
            g_process.Destroy();
            g_process = lldb::SBProcess();
        }
        
        if (g_target.IsValid())
            g_target = lldb::SBTarget();
        
        if (g_debugger.IsValid()) {
            lldb::SBDebugger::Destroy(g_debugger);
            g_debugger = lldb::SBDebugger();
        }
        
        g_debuggerInitialized = false;
    }

    VariableInfo getValueOfVariable(dynvar* variableName) {
        VariableInfo info;
        info.variableName = nullptr;
        info.value.address = 0;
        info.value.length = 0;
        info.pid = -1;

        if (!g_debuggerInitialized) {
            bugDetected("Debugger not initialized. Call startDebugger() first.");
            return info;
        }

        if (!g_process.IsValid()) {
            bugDetected("No active debugging process.");
            return info;
        }

        // Get the PID from the process
        info.pid = g_process.GetProcessID();

        // Get the current thread
        lldb::SBThread thread = g_process.GetSelectedThread();
        if (!thread.IsValid()) {
            bugDetected("No valid thread found.");
            return info;
        }

        // Get the current frame
        lldb::SBFrame frame = thread.GetSelectedFrame();
        if (!frame.IsValid()) {
            bugDetected("No valid frame found.");
            return info;
        }

        if (!(char*)getValue(*variableName)) {
            bugDetected("Invalid variable name provided.");
            return info;
        }

        // Get variables from the current frame
        lldb::SBValueList variables = frame.GetVariables(true, true, false, true);
        size_t numVariables = variables.GetSize();

        for (size_t i = 0; i < numVariables; ++i) {
            lldb::SBValue variable = variables.GetValueAtIndex(i);
            if (variable.IsValid()) {
                const std::string name = variable.GetName();
                if (!name.empty() && strcmp(name.c_str(), (char*)getValue(*variableName)) == 0) {
                    // Found the variable
                    const std::string valueStr = variable.GetValue();
                    
                    // Copy variable name
                    info.variableName = strdup(name.c_str());
                    
                    // Copy value using dynvar
                    if (valueStr.c_str())
                        setValue(&info.value, (char*)valueStr.c_str());
                    else
                        setValue(&info.value, (char*)"<no value>");
                    
                    return info;
                }
            }
        }
        bugDetected(((std::string)"Variable '" + std::string((char*)getValue(*variableName)) + "' not found in current frame.").c_str());
        return info;
    }

    SymInfo getLineNumber(uint64_t address) {
        SymInfo info;
        info.lineNumber = -1;
        info.sourcePath.address = 0;
        info.sourcePath.length = 0;

        if (!g_debuggerInitialized) {
            bugDetected("Debugger not initialized. Call startDebugger() first.");
            return info;
        }

        if (!g_target.IsValid()) {
            bugDetected("No valid target.");
            return info;
        }

        // Check if target has loaded symbols/modules
        uint32_t num_modules = g_target.GetNumModules();
        if (num_modules == 0) {
            bugDetected("No symbols loaded. Target has no modules.");
            return info;
        }

        lldb::SBAddress addr(address, g_target);
        if (!addr.IsValid()) {
            bugDetected("Invalid address or address not in any loaded module.");
            return info;
        }

        lldb::SBLineEntry lineEntry = addr.GetLineEntry();
        if (!lineEntry.IsValid()) {
            bugDetected("Address does not map to any source line (symbols may be missing).");
            return info;
        }

        info.lineNumber = lineEntry.GetLine();

        lldb::SBFileSpec fileSpec = lineEntry.GetFileSpec();
        if (fileSpec.IsValid()) {
            char path_buf[PATH_MAX];
            uint32_t path_len = fileSpec.GetPath(path_buf, sizeof(path_buf));
            if (path_len > 0 && path_len < sizeof(path_buf))
                setValue(&info.sourcePath, path_buf);
        }

        return info;
    }

    VariableInfo getReturnValue() {
        VariableInfo info;
        info.variableName = nullptr;
        info.value.address = 0;
        info.value.length = 0;
        info.pid = -1;

        if (!g_debuggerInitialized) {
            bugDetected("Debugger not initialized. Call startDebugger() first.");
            return info;
        }

        if (!g_process.IsValid()) {
            bugDetected("No active debugging process.");
            return info;
        }

        // Get the PID from the process
        info.pid = g_process.GetProcessID();

        // Get the current thread
        lldb::SBThread thread = g_process.GetSelectedThread();
        if (!thread.IsValid()) {
            bugDetected("No valid thread found.");
            return info;
        }

        // Get the current frame
        lldb::SBFrame frame = thread.GetSelectedFrame();
        if (!frame.IsValid()) {
            bugDetected("No valid frame found.");
            return info;
        }

        // Try to get the return value from the thread
        lldb::SBValue returnValue = thread.GetStopReturnValue();
        if (returnValue.IsValid()) {
            const std::string valueStr = returnValue.GetValue();
            
            // Copy variable name as "return"
            info.variableName = strdup("return");
            
            // Copy value
            if (valueStr.c_str())
                setValue(&info.value, (char*)valueStr.c_str());
            else
                setValue(&info.value, (char*)"<no value>");
            
            return info;
        }

        bugDetected("Unable to retrieve return value from current context.");
        return info;
    }
}

static bool bpCallback(void *baton, lldb::SBProcess &process, lldb::SBThread &thread, lldb::SBBreakpointLocation &location) {
    lldb::break_id_t bp_id = location.GetBreakpoint().GetID();
    auto it = g_breakpointCallbacks.find(bp_id);
    if (it != g_breakpointCallbacks.end() && it->second) {
        BreakpointCallback callback = (BreakpointCallback)it->second;
        lldb::SBBreakpoint bp = location.GetBreakpoint();
        lldb::SBLineEntry lineEntry = bp.GetLocationAtIndex(0).GetAddress().GetLineEntry();
        int line = lineEntry.GetLine();
        lldb::SBFileSpec fileSpec = lineEntry.GetFileSpec();
        char path_buf[PATH_MAX];
        uint32_t path_len = fileSpec.GetPath(path_buf, sizeof(path_buf));
        const std::string sourceFile = (path_len > 0 && path_len < sizeof(path_buf)) ? path_buf : "";
        callback(line, sourceFile);
    }
    return false;
}

dynvar getNodeValue(AST* node, DATATYPE* dtype) {
    dynvar val;
    val.address = 0;
    val.length = 0;

    if (auto valNode = dynamic_cast<ValueNode*>(node)) {
        setValue(&val, (char*)valNode->value.c_str());
        *dtype = STRING;
    } else if (auto boolNode = dynamic_cast<BooleanNode*>(node)) {
        //
    }

    return val;
}

lldb::SBBreakpoint addBreakpoint(int line, dynvar* source, void* callback) {
    lldb::SBBreakpoint invalid_bp;

    if (!g_debuggerInitialized) {
        bugDetected("Debugger not initialized. Call startDebugger() first.");
        return invalid_bp;
    }

    if (!g_target.IsValid()) {
        bugDetected("No valid target for breakpoint.");
        return invalid_bp;
    }

    if (!source || source->address == 0) {
        bugDetected("Source file variable is empty.");
        return invalid_bp;
    }

    long* sourcePathPtr = getValue(*source);
    if (!sourcePathPtr || source->length == 0) {
        bugDetected("Source file path is empty or invalid.");
        return invalid_bp;
    }

    const std::string sourcePath = (const char*)sourcePathPtr;
    lldb::SBBreakpoint bp = g_target.BreakpointCreateByLocation(sourcePath.c_str(), line);
    if (!bp.IsValid()) {
        bugDetected("Failed to create breakpoint.");
        return invalid_bp;
    }

    bp.SetEnabled(true);

    if (callback) {
        g_breakpointCallbacks[bp.GetID()] = callback;
        bp.SetCallback(bpCallback, nullptr);
    }

    return bp;
}

void deleteBreakpoint(lldb::SBBreakpoint bp) {
    if (!g_debuggerInitialized) {
        bugDetected("Debugger not initialized. Call startDebugger() first.");
        return;
    }

    if (!g_target.IsValid()) {
        bugDetected("No valid target.");
        return;
    }

    if (!bp.IsValid()) {
        bugDetected("Invalid breakpoint.");
        return;
    }

    lldb::break_id_t bp_id = bp.GetID();
    g_target.BreakpointDelete(bp_id);
    g_breakpointCallbacks.erase(bp_id);
}