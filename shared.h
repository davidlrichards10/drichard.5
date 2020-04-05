#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>

typedef struct clock {
	int seconds;
	int nanoSeconds;
} Clock;

struct mesgQ {
    long mType;
    char mText[100];
} messenger;

typedef struct {
	int max[20];
	int available[20];
} Resource;

typedef struct {
       int allocated[20];
       int request[20];
	int isTerminated;
} ResourceDescriptor;

typedef struct {
	int fakePid;
	int deadLockResources[20];
} DeadLock;

typedef struct shared_memory_object {
    ResourceDescriptor resourceDescriptor[18 + 1];
    DeadLock deadLock[18 + 1];
    Resource resources;
    Clock clockInfo;
} SharedMemory; 
