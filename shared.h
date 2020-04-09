#ifndef SHARED_H
#define SHARED_H

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
	int termF;
        int requestF;
        int releaseF;
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

#endif
