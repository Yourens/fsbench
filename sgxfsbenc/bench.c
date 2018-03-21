#include "common.h"
#include "enclave.h"
#include "enclave_t.h"
#include "fs.h"
#include "sgx_tprotected_fs.h"
#include "sgx_trts.h"
#include <string.h>

static int nu_blocks;

static int *indices;
static char *buffer;
static char test_filename[] = "Testfile.txt";
static int base_size = 256;
static int block_size = 0;

void ecall_CreateTestFile(int nu_blocks, int blocksize) {
	block_size = base_size << blocksize;
  buffer = (char *)malloc(sizeof(char) * block_size);
  memcpy(buffer, "Hello", 6);
  SGX_FILE *fd = file_open(test_filename, "w+");
  for (int i = 0; i < nu_blocks; i++)
    file_write_with_check(fd, buffer, block_size);
  file_close(fd);
  free(buffer);
}
static void CheckFDError(SGX_FILE *fd) {
  /* if (fd < 0) */
  /* FatalError("Error opening file.\n"); */
}

static int SequentialRead() {
  SGX_FILE *fd = file_open(test_filename, "r");
  CheckFDError(fd);
  for (int i = 0; i < nu_blocks; i++) {
    file_read_with_check(fd, buffer, block_size);
  }
  file_close(fd);
  return nu_blocks;
}

static int SequentialWrite() {
  SGX_FILE *fd = file_open(test_filename, "w");
  CheckFDError(fd);
  for (int i = 0; i < nu_blocks; i++)
    file_write_with_check(fd, buffer, block_size);
  file_close(fd);
  return nu_blocks;
}

static int RandomRead() {
  SGX_FILE *fd = file_open(test_filename, "r");
  CheckFDError(fd);
  for (int i = 0; i < nu_blocks; i++) {
    int block_index = indices[i];
    sgx_fseek(fd, (long)block_index * block_size, SEEK_SET);
    file_read_with_check(fd, buffer, block_size);
  }
  file_close(fd);
  return nu_blocks;
}

static int RandomWrite() {
  SGX_FILE *fd = file_open(test_filename, "w");
  CheckFDError(fd);
  for (int i = 0; i < nu_blocks; i++) {
    int block_index = indices[i];
    sgx_fseek(fd, (long)block_index * block_size, SEEK_SET);
    file_write_with_check(fd, buffer, block_size);
  }
  file_close(fd);
  return nu_blocks;
}

void ecall_run_benchmark(int pattern, int _nu_blocks, int *_indices) {
  indices = _indices;

  nu_blocks = _nu_blocks;
	//base size is 256
  buffer = (char *)malloc(sizeof(char) * block_size);

  switch (pattern) {
  case Seq_Read:
    SequentialRead();
    break;
  case Seq_Write:
    SequentialWrite();
    break;
  case Random_Read:
    RandomRead();
    break;
  case Random_Write:
    RandomWrite();
    break;
  default:
    return;
  }
  free(buffer);
}

void ecall_empty() { return; }
