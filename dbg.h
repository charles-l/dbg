// LINUX DEBUGGER UTILS
#ifndef DBG_H
#define DBG_H
#include <sys/uio.h>

const int WORD_SIZE = 8; // x86_64

int read_mem(pid_t pid, off_t start, char *buf, size_t n) {
  struct iovec local[1];
  struct iovec remote[1];

  local[0].iov_base = buf;
  local[0].iov_len = n;

  remote[0].iov_base = (void *) start;
  remote[0].iov_len = n;

  return process_vm_readv(pid, local, 1, remote, 1, 0);
}

struct pmap {
  unsigned long begin;
  unsigned long end;
  unsigned long size;
  unsigned long inode;
  char perm[5];
  char dev[6];
  char mapname[64];
};

// this is utter crap
struct pmap *read_mem_maps(pid_t pid, int *n) {
  char map_path[64];
  FILE *f;
  *n = 16;

  struct pmap *pmaps = (struct pmap *) malloc(sizeof(struct pmap) * (*n));

  sprintf(map_path, "/proc/%ld/maps", (long)pid);
  f = fopen(map_path, "r");

  if(!f) return NULL;

  char buf[128];
  for(int i = 0; !feof(f); i++) {
    struct pmap m;

    if(fgets(buf, sizeof(buf), f) == 0) break;

    m.mapname[0] = '\0';
    int t;
    sscanf(buf, "%lx-%lx %4s %lx %5s %ld %s",
        &m.begin, &m.end, m.perm,
        &t, m.dev, &m.inode, m.mapname);

    m.size = m.end - m.begin;

    if(i >= *n - 1) {
      *n *= 2;
      pmaps = (struct pmap *) realloc(pmaps, sizeof(struct pmap) * (*n));
    }

    pmaps[i] = m;
  }
  fclose(f);
  return pmaps;
}

// `n` must be divisible by word_size
void dump_hex(char *buf, size_t n) {
    for(int i = 0; i < n / WORD_SIZE; i++) {
        for(int j = 0; j < WORD_SIZE; j++) {
            fprintf(stderr, "%02hhX ", buf[(i * WORD_SIZE) + j]);
        }
        fprintf(stderr, "\n");
    }
}

#endif // DBG_H
