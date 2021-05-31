#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <errno.h>

int counter = 0;

int main(int argc, char* argv[])
{/*
	if(ptrace(PTRACE_TRACEME, 0, 0) < 0)
	{
        perror("PTRACE_TRACEME");
        return errno;
    }*/
	prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY , 0, 0, 0);
	//prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, CAP_SYS_PTRACE);
	
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 1e8};
	for(counter = 0; ; counter++)
	{
		printf("%d\n", counter);
		nanosleep(&ts, NULL);
	}
}
