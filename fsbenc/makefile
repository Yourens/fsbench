
LDFLAGS := -lstdc++ 

INCLUDE_PATH := 
CFLAGS := $(INCLUDE_PATH) -Wall -g -O0
 
CXX = g++
CC = gcc
LINKER = $(CC)
AR = ar cru

#make rules
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@	

%.o : %.cc
	$(CC) $(CFLAGS) -c $< -o $@	

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@	



OBJS = bench.o \
			 fs.o \
			 app.o

TARGETS = bench 

all: $(TARGETS)

bench: $(OBJS)
	$(LINKER) $^ $(LDFLAGS) -o $@

clean:
	@($(RM) $(TARGETS))
	@($(RM) *.o)
	@($(RM) Testfile.txt)

