#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/prctl.h>
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <errno.h>

int counter = 0;

int main(int argc, char* argv[])
{
	pid_t child;
	struct user_regs_struct uregs;
	child = fork();
	if(child == 0)
	{
		printf("Started child!\n");
		struct timespec ts = {.tv_sec = 0, .tv_nsec = 1e8};
		for(counter = 0; counter < 20; counter++)
		{
			printf("child: %d\n", counter);
			nanosleep(&ts, NULL);
		}
	}
	else
	{
		struct timespec ts = {.tv_sec = 0, .tv_nsec = 2e8};
		struct iovec loc;
		struct iovec rem;
		ssize_t bufferLength = 1024;
		loc.iov_base = calloc(bufferLength, 1);
		loc.iov_len = bufferLength;
		rem.iov_base = &counter;
		rem.iov_len = sizeof(counter);

		printf("Child pid: %d\n", child);
		
		nanosleep(&ts, NULL);

		if(ptrace(PTRACE_ATTACH, child, NULL, NULL) < 0)
		{
			perror("PTRACE_ATTACH");
			return -errno;
		}

		int status = 0;
		while(1)
		{
			wait(&status);
			if(WIFSTOPPED(status))
				break;
		}				

		int64_t val = ptrace(PTRACE_PEEKDATA, child, &counter, NULL);
		ptrace(PTRACE_CONT, child, NULL, NULL);
		printf("parent has read: %ld\n", val);

		ssize_t nread = process_vm_readv(child, &loc, 1, &rem, 1, 0);

		if(nread < 0)
		{
			perror("process_vm_readv");
			return errno;
		}

		printf("process_vm_readv: %d\n", *((int*)loc.iov_base));
		wait(NULL);
		printf("Done!\n");
	}
}
