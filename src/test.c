#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

uint64_t counter = 0;

int main(int argc, char *argv[]) {
  pid_t child;
  struct user_regs_struct uregs;
  child = fork();
  if (child == 0) {
    printf("Started child!\n");
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 1e8};
    for (counter = 0; counter < 20; counter++) {
      printf("child: %d\n", counter);
      nanosleep(&ts, NULL);
    }
  } else {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 2e8};
    struct iovec loc;
    struct iovec rem;
    ssize_t bufferLength = 1024;
    loc.iov_base = calloc(bufferLength, 1);
    loc.iov_len = bufferLength;
    rem.iov_base = &counter;
    rem.iov_len = sizeof(counter);

    printf("Child pid: %d. Counter address: %p\n", child, &counter);

    nanosleep(&ts, NULL);

    if (ptrace(PTRACE_ATTACH, child, NULL, NULL) < 0) {
      perror("PTRACE_ATTACH");
      // return -errno;
    } else {
      int status = 0;
      while (1) {
        wait(&status);
        if (WIFSTOPPED(status))
          break;
      }

      uint64_t val = ptrace(PTRACE_PEEKDATA, child, &counter, NULL);
      ptrace(PTRACE_CONT, child, NULL, NULL);
      printf("ptrace has read: %lu\n", val);
    }

    ssize_t nread = process_vm_readv(child, &loc, 1, &rem, 1, 0);

    if (nread < 0) {
      perror("process_vm_readv");
      // return -errno;
    } else {
      printf("process_vm_readv: %d\n", *((int *)loc.iov_base));
    }

    int fd = 0;
    char path_fmt[] = "/proc/%d/mem";
    char path[1024];
    snprintf(&path[0], 1024, &path_fmt[0], child);
    fd = open(path, O_RDONLY);

    uint64_t counter_read = 0;
    uint64_t byte_offset = (uint64_t)&counter;
    if (byte_offset != lseek(fd, byte_offset, SEEK_SET)) {
      perror("lseek");
      return -errno;
    }
    read(fd, &counter_read, sizeof(counter));
    close(fd);

    printf("read from mem file: %lu\n", counter_read);

    free(loc.iov_base);
    wait(NULL);
    printf("Done!\n");
  }
}
