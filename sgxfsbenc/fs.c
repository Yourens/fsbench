#include "sgx_trts.h"

#include <stdarg.h>
#include <stdio.h> /* vsnprintf */

#include "enclave.h"
#include "enclave_t.h"
#include "errno.h"
#include "sgx_error.h"
#include "sgx_tprotected_fs.h"
#include <ctype.h>
#include <sgx_tcrypto.h>
#include <sgx_tseal.h>
#include <sgx_utils.h>
#include <stdlib.h>
#include <string.h>

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */

void printf(const char *fmt, ...) {
  char buf[BUFSIZ] = {'\0'};
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string(buf);
}

SGX_FILE *file_open(const char *filename, const char *mode) {
  SGX_FILE *a;
  a = sgx_fopen_auto_key(filename, mode);
  if (a == NULL) {
    printf("Open file %s failed, errno: %x", filename, errno);
  }
  return a;
}

size_t file_write_with_check(SGX_FILE *fp, char *data, size_t size) {
  size_t sizeofWrite;
  sizeofWrite = sgx_fwrite(data, sizeof(char), size, fp);
  if (sizeofWrite != size) {
    printf("write file failed, errno: %x", errno);
  }
  return sizeofWrite;
}

size_t file_read_with_check(SGX_FILE *fp, char *readData, uint64_t size) {

  size_t sizeofRead = sgx_fread(readData, sizeof(char), size, fp);

  /* printf("%s\n", readData); */
  if (sizeofRead != size) {
    printf("read file failed, errno: %x", errno);
  }

  return sizeofRead;
}

int32_t file_close(SGX_FILE *fp) {
  int32_t a;
  int r;
flush:
  r = sgx_fflush(fp);
  if (r != 0)
    goto flush;
  a = sgx_fclose(fp);
  return a;
}
