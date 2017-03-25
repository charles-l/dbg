// LINUX DEBUGGER UTILS
#ifndef DBG_H
#define DBG_H
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

const int WORD_SIZE = 8; // x86_64

struct mem_file {
  int fd;
  int pid;
};

struct mem_file mem_file_load(long pid) {
  char buf[64];
  sprintf(buf, "/proc/%d/mem", pid);
  struct mem_file m;
  m.pid = pid;
  m.fd = open(buf, O_RDONLY);
  ptrace(PTRACE_ATTACH, m.pid, NULL, NULL);
  waitpid(m.pid, NULL, 0);
  return m;
}

void mem_file_close(struct mem_file *m) {
  ptrace(PTRACE_DETACH, m->pid, NULL, NULL);
  close(m->fd);
}

void mem_file_read(struct mem_file *m, off_t oaddr, char *buf, size_t n) {
    // TODO: try process_vm_readv
    pread(m->fd, buf, n, oaddr);
}

// `n` must be divisible by word_size
void dump_hex(char *buf, size_t n, long start) {
    for(int i = 0; i < n / WORD_SIZE; i++) {
        printf("0x%08x ", start + (i * WORD_SIZE));
        for(int j = 0; j < WORD_SIZE; j++) {
            printf("%02hhX ", buf[(i * WORD_SIZE) + j]);
        }
        putchar('\n');
    }
}

#endif // DBG_H
