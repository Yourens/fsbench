#include <assert.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_PATH FILENAME_MAX

#include "app.h"
#include "common.h"
#include "enclave_u.h"
#include "sgx_uae_service.h"
#include "sgx_urts.h"

#define MAX_BUF_LEN 100
/* Global EID shared by multiple threads */
sgx_enclave_id_t eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED, "Unexpected error occurred.", NULL},
    {SGX_ERROR_INVALID_PARAMETER, "Invalid parameter.", NULL},
    {SGX_ERROR_OUT_OF_MEMORY, "Out of memory.", NULL},
    {SGX_ERROR_ENCLAVE_LOST, "Power transition occurred.",
     "Please refer to the sample \"PowerTransition\" for details."},
    {SGX_ERROR_INVALID_ENCLAVE, "Invalid enclave image.", NULL},
    {SGX_ERROR_INVALID_ENCLAVE_ID, "Invalid enclave identification.", NULL},
    {SGX_ERROR_INVALID_SIGNATURE, "Invalid enclave signature.", NULL},
    {SGX_ERROR_OUT_OF_EPC, "Out of EPC memory.", NULL},
    {SGX_ERROR_NO_DEVICE, "Invalid SGX device.",
     "Please make sure SGX module is enabled in the BIOS, and install SGX "
     "driver afterwards."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT, "Memory map conflicted.", NULL},
    {SGX_ERROR_INVALID_METADATA, "Invalid enclave metadata.", NULL},
    {SGX_ERROR_DEVICE_BUSY, "SGX device was busy.", NULL},
    {SGX_ERROR_INVALID_VERSION, "Enclave version was invalid.", NULL},
    {SGX_ERROR_INVALID_ATTRIBUTE, "Enclave was not authorized.", NULL},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS, "Can't open enclave file.", NULL},
    {SGX_ERROR_NDEBUG_ENCLAVE, "The enclave is signed as product enclave, and "
                               "can not be created as debuggable enclave.",
     NULL},
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if (ret == sgx_errlist[idx].err) {
      if (NULL != sgx_errlist[idx].sug)
        printf("Info: %s\n", sgx_errlist[idx].sug);
      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }

  if (idx == ttl)
    printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void) {
  char token_path[MAX_PATH] = {'\0'};
  sgx_launch_token_t token = {0};
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;
  int updated = 0;

  /* Step 1: try to retrieve the launch token saved by last transaction
   *         if there is no token, then create a new one.
   */

  /* try to get the token saved in $HOME */
  const char *home_dir = getpwuid(getuid())->pw_dir;

  if (home_dir != NULL &&
      (strlen(home_dir) + strlen("/") + sizeof(TOKEN_FILENAME) + 1) <=
          MAX_PATH) {
    /* compose the token path */
    strncpy(token_path, home_dir, strlen(home_dir));
    strncat(token_path, "/", strlen("/"));
    strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME) + 1);
  } else {
    /* if token path is too long or $HOME is NULL */
    strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
  }

  FILE *fp = fopen(token_path, "rb");
  if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
    printf("Warning: Failed to create/open the launch token file \"%s\".\n",
           token_path);
  }

  if (fp != NULL) {
    /* read the token from saved file */
    size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
    if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
      /* if token is invalid, clear the buffer */
      memset(&token, 0x0, sizeof(sgx_launch_token_t));
      printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
    }
  }

  /* Step 2: call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated,
                           &eid, NULL);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);

    if (fp != NULL)
      fclose(fp);
    return -1;
  }

  /* Step 3: save the launch token if it is updated */

  if (updated == FALSE || fp == NULL) {
    /* if the token is not updated, or file handler is invalid, do not perform
     * saving */
    if (fp != NULL)
      fclose(fp);
    return 0;
  }

  /* reopen the file with write capablity */
  fp = freopen(token_path, "wb", fp);
  if (fp == NULL)
    return 0;
  size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
  if (write_num != sizeof(sgx_launch_token_t))
    printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
  fclose(fp);
  return 0;
}

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
}

// void ocall_dimatcopy(const char ordering, const char trans, size_t rows,
// size_t cols, const double alpha, double * AB, size_t lda, size_t ldb, size_t
// size)
//{
//    mkl_dimatcopy(ordering, trans, rows, cols, alpha, AB, lda, ldb);
//}

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
int SGX_CDECL main(int argc, char *argv[]) {
  uint64_t skew;
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
  if (initialize_enclave() < 0) {
    printf("Enter a character before exit ...\n");
    getchar();
    return -1;
  }

  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

  SGX_FILE *fp;

  CreateIndices();
  SetRandomIndices();

  starttime = GetCurrentTimeUSec();
  for (int i = 0; i < 10000; i++) {
    ecall_empty(eid);
  }
  endtime = GetCurrentTimeUSec();
  skew = (endtime - starttime) / 10000;
  printf("Skew : %ld\n", skew);

  ecall_CreateTestFile(eid, nu_blocks, blocksize);

  starttime = GetCurrentTimeUSec();
  ecall_run_benchmark(eid, pattern, nu_blocks, indices);
  Sync();
  DropCaches();
  endtime = GetCurrentTimeUSec();
  double cost = ((endtime - starttime) - skew) / (double)1000000;
  printf("cost: %f s\n", cost);
  printf("speed: %f MB/s\n", totalsize / cost);

  /* Destroy the enclave */
  sgx_destroy_enclave(eid);

	remove("Testfile.txt");
  return 0;
}
