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

#include <stdio.h>
#include <stdbool.h>
#include <seccomp.h>
#include <seccomp-syscalls.h>
#include <stdlib.h>

#include <debugger.h>
#include <runtime.h>
#include <definations.h>
#include <ai.h>
#include <dynvar.h>

#ifndef EPERM
#define EPERM 1
#endif

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

        //bhResult result = guessTheBehavior(registers, syscall_num, arch);
        bhResult result = OKAY;

        // Prepare response to resume or block the sandboxed process
        resp->id = req->id;
        resp->flags = 0;

        // Allow syscall if AI determines it's safe, otherwise block
        if (result == OKAY) {
            resp->error = 0; // Allow the syscall
            resp->val = 0;
        } else {
            resp->error = -EPERM; // Block the syscall (return Permission Denied)
            resp->val = 0;
        }

        // Send the decision back to the kernel to unpause the tracee
        seccomp_notify_respond(notify_fd, resp);
    }

    seccomp_notify_free(req, resp);
}