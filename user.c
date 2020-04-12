#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>

int shmid; 
int child_id;
int chance[100];
int chancePos =0;
SharedMemory* ptr;
void addClock(struct time* time, int sec, int ns);

int main(int argc, char* argv[]) {

     	if ((shmid = shmget(9784, sizeof(SharedMemory), 0600)) < 0) 
	{
            perror("Error: shmget");
            exit(errno);
     	}
   
	ptr = shmat(shmid, NULL, 0);

	srand(getpid());

	while(1) 
	{
	
		int chance = rand() % (100 + 1 - 1) + 1;
		int chance1 = rand() % (100 + 1 - 1) + 1;

		if(chance > 0 && chance < 63) 
		{
			ptr->resourceStruct.requestF = 1;
			int resourceIndex = rand() % (19 + 0 - 0) + 0;
			ptr->resourceStruct.index = resourceIndex;
		} 
		else 
		{
			ptr->resourceStruct.releaseF = 1;
		}
		if(chance1 < 10) 
		{	
			ptr->resourceStruct.termF = 1;
		}
		exit(0);
	

	}	
	return 0;
}

void addClock(struct time* time, int sec, int ns){
	time->seconds += sec;
	time->nanoseconds += ns;
	while(time->nanoseconds >= 1000000000){
		time->nanoseconds -=1000000000;
		time->seconds++;
	}
}
