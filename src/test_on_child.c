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

void child_func() {
  printf("Started child!\n");
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 1e8};
  for (counter = 0;; counter++) {
    printf("###### child: %lu\n", counter);
    nanosleep(&ts, NULL);
  }
}

void ptrace_test(pid_t pid) {
  static int attached = 0;
  if (!attached) {
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
      perror("PTRACE_ATTACH");
      // return -errno;
    } else {
      int status = 0;
      while (1) {
        wait(&status);
        if (WIFSTOPPED(status))
          break;
      }
      attached = 1;
    }

    uint64_t val = ptrace(PTRACE_PEEKDATA, pid, &counter, NULL);
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    // printf("ptrace has read: %lu\n", val);
  }
}

#define LOOPS 1000

void process_vm_readv_test(pid_t pid) {
  clock_t start, end;
  start = clock();

  for (uint64_t i = 0; i < LOOPS; i++) {
    struct iovec loc;
    struct iovec rem;
    ssize_t bufferLength = 1024;
    loc.iov_base = calloc(bufferLength, 1);
    loc.iov_len = bufferLength;
    rem.iov_base = &counter;
    rem.iov_len = sizeof(counter);

    ssize_t nread = process_vm_readv(pid, &loc, 1, &rem, 1, 0);
    /*
      if (nread < 0) {
        perror("process_vm_readv");
        // return -errno;
      } else {
        printf("process_vm_readv: %d\n", *((int *)loc.iov_base));
      }
    */
    free(loc.iov_base);
  }

  end = clock();
  double total = ((double)(end - start) / CLOCKS_PER_SEC);
  printf("\nprocess_vm_readv_test: %.4e\n\n", total / LOOPS);
}

void direct_read_test(pid_t pid) {
  clock_t start, end;
  start = clock();

  int fd = 0;
  char path_fmt[] = "/proc/%d/mem";
  char path[1024];
  snprintf(&path[0], 1024, &path_fmt[0], pid);

  fd = open(path, O_RDONLY);
  uint64_t byte_offset = (uint64_t)&counter;
  lseek(fd, byte_offset, SEEK_SET);

  for (uint64_t i = 0; i < LOOPS; i++) {
    uint64_t counter_read = 0;

    /*
      if (byte_offset != lseek(fd, byte_offset, SEEK_SET)) {
        perror("lseek");
        // return -errno;
      }
    */
    read(fd, &counter_read, sizeof(counter));

    // printf("read from mem file: %lu\n", counter_read);
  }
  close(fd);

  end = clock();
  double total = ((double)(end - start) / CLOCKS_PER_SEC);
  printf("\ndirect_read_test: %.4e\n\n", total / LOOPS);
}

int main(int argc, char *argv[]) {
  pid_t child;
  struct user_regs_struct uregs;
  child = fork();
  if (child == 0) {
    child_func();
  } else {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 2e8};
    printf("Child pid: %d. Counter address: %p\n", child, &counter);
    nanosleep(&ts, NULL);

    // ptrace_test(child);
    process_vm_readv_test(child);
    direct_read_test(child);

    kill(child, SIGKILL);
    wait(NULL);
    printf("Done!\n");
  }
}
