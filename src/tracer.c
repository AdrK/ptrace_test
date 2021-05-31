#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <errno.h>
#include <sys/uio.h>

uint64_t counter = 0;

int main(int argc, char* argv[])
{
    pid_t pid = strtol(argv[1], NULL, 10);
    void *addr = (void*)strtoul(argv[2], NULL, 16);
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
    {
        perror("PTRACE_ATTACH");
        return errno;
    }
    waitpid(pid, NULL, 0);
    
    printf("Started!\n");
    
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    printf("Read reg value: %llu\n", regs.orig_rax);

    struct user* user_space = (struct user*)0;
    long original_rax = ptrace(PTRACE_PEEKUSER, pid, &user_space->regs.orig_rax, NULL);
    printf("Read userspace reg value: %llu\n", regs.orig_rax);

    struct iovec loc;
    struct iovec rem;
    size_t bufferLength = sizeof(uint64_t);
    loc.iov_base = calloc(bufferLength, sizeof(char));
    loc.iov_len = bufferLength;

    rem.iov_base = addr;
    rem.iov_len = bufferLength;

    ssize_t nread = process_vm_readv(pid, &loc, 1, &rem, 1, 0);
    if(nread < 0)
    {
        perror("process_vm_readv");
        return errno;
    }

    int64_t val = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
    if(val < 0)
    {
        perror("PTRACE_PEEKDATA");
        return errno;
    }

    printf("Read value: %ld\n", val);
    free(loc.iov_base);
}
//./tracer `pidof tracee` `objdump -Tt ./tracee  | awk '/ counter$/{print $1; exit}'`
