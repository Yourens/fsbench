
#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <stdint.h>

#include <string.h> 
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

FILE* file_open(const char* filename, const char* mode) 
{
	FILE* a;
	a = fopen(filename, mode);
	if(a == NULL){
		printf("Open file %s failed, errno: %x", filename, errno);
	}
	return a;
}

size_t file_write_with_check(FILE* fp, char *data, size_t size) 
{
	size_t sizeofWrite;
	sizeofWrite = fwrite(data, sizeof(char), size, fp);
	if(sizeofWrite != size){
		printf("write file failed, errno: %x",  errno);
	}
	return sizeofWrite;
}

size_t file_read_with_check(FILE* fp, char* readData, uint64_t size) 
{

	size_t sizeofRead = fread(readData, sizeof(char), size, fp);

	if(sizeofRead != size){
		printf("read file failed, errno: %x", errno);
	}

	return sizeofRead;
}

int32_t file_close(FILE* fp)
{
	int32_t a;
	int r;
flush:
	r = fflush(fp);
	if(r != 0)
		goto flush;
	a = fclose(fp);
	return a;
}
