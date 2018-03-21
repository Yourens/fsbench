#ifndef __FS_H__


FILE* file_open(const char* filename, const char* mode);
void file_read_with_check(FILE* fd, void *buffer, size_t size) ;
void file_write_with_check(FILE* fd, void *buffer, size_t size);
int32_t file_close(FILE* fp);
#endif
