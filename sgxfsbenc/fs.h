#ifndef __FS_H__

void ecall_enclaveString(char *s, size_t len);
SGX_FILE* file_open(const char* filename, const char* mode);
size_t file_write_with_check(SGX_FILE* fp, char *data, size_t size);
size_t file_read_with_check(SGX_FILE* fp, char* readData, uint64_t size);
int32_t file_close(SGX_FILE* fp);

#endif
