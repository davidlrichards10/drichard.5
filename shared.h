#ifndef SHARED_H
#define SHARED_H

struct time{
	int nanoseconds;
	int seconds;
};

typedef struct {
       	int allocated[20];
       	int request[20];
	int max[20];
        int available[20];
        int termF;
        int requestF;
        int releaseF;
} resourceDescriptor;

typedef struct {
	int max[20];
        int available[20];
        int termF;
        int requestF;
        int releaseF;
} resourceInfo;

typedef struct shared_memory_object {
    	resourceDescriptor descriptor[18 + 1];
	resourceInfo resourceStruct;
	struct time time;
} SharedMemory; 

#endif
