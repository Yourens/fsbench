#include <assert.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#define MAX_PATH FILENAME_MAX

#include "app.h"
#include "common.h"

#define MAX_BUF_LEN 100

void ocall_print_uint(uint8_t *u, size_t size) {
  printf("Info: uint8_t*: ");
  for (int i = 0; i < size; i++) {
    if (i % 24 == 0)
      printf("\n");
    printf("%4d", (uint8_t) * (u + i));
  }
  printf("\n");
}

static int *indices;
static int nu_blocks;
static const char *empty_environment[] = {NULL};
static const char char_three = '3';

static void CreateIndices() {
  indices = (int *)malloc(sizeof(int) * nu_blocks);
}

static void SetRandomIndices() {
  for (int i = 0; i < nu_blocks; i++)
    indices[i] = i;
  // Traverse the array from start to end and swap indices randomly.
  for (int i = 0; i < nu_blocks; i++) {
    int j = rand() % nu_blocks;
    int index_i = indices[i];
    indices[i] = indices[j];
    indices[j] = index_i;
  }
}

static void ExecuteSync() {
  // Create a child process
  int child_pid = fork();
  if (child_pid == 0)
    execle("/bin/sync", "/bin/sync", (char *)NULL, empty_environment);
  int status;
  waitpid(child_pid, &status, 0);
}

static void DropCaches() {
  ExecuteSync();
  int fd = open("/proc/sys/vm/drop_caches", O_WRONLY | O_SYNC);
  int r = write(fd, &char_three, 1);
  close(fd);
  if (r != 1)
    printf("Warning: Cache flushing unsuccesful. Permission problem?\n"
           "bench should be run as superuser for best effects.\n");
}

static void Sync() { ExecuteSync(); }

uint64_t GetCurrentTimeUSec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void printhelp() { printf("./bench [sR|sW|rR|rW] blocksize\n"); }

/* Application entry */
int main(int argc, char *argv[]) {
  uint64_t starttime, endtime;
  int pattern = -1;

  if (argc != 3) {
    printhelp();
    exit(0);
  }
  if (strncmp(argv[1], "sR", 3) == 0)
    pattern = Seq_Read;

  if (strncmp(argv[1], "sW", 3) == 0)
    pattern = Seq_Write;
  if (strncmp(argv[1], "rR", 3) == 0)
    pattern = Random_Read;
  if (strncmp(argv[1], "rW", 3) == 0)
    pattern = Random_Read;
	char * c;
	int blocksize = strtol(argv[2], &c, 0);
  nu_blocks = 6400000>>(blocksize);

  int totalsize = nu_blocks >> (12 - blocksize);

  printf("file size : %d MB\n", totalsize);

  /* Initialize the enclave */

  CreateIndices();
  SetRandomIndices();

  ecall_CreateTestFile(nu_blocks, blocksize);

  starttime = GetCurrentTimeUSec();
  ecall_run_benchmark(pattern, nu_blocks, indices);
  Sync();
  DropCaches();
  endtime = GetCurrentTimeUSec();
  double cost = (endtime - starttime)/(double)1000000;
  printf("cost: %f s\n", cost);
  printf("speed: %f MB/s\n", totalsize / cost);

  return 0;
}
