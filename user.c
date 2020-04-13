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

int main(int argc, char* argv[]) 
{
     	if ((shmid = shmget(9784, sizeof(SharedMemory), 0600)) < 0) 
	{
            perror("Error: shmget");
            exit(errno);
     	}
   
	ptr = shmat(shmid, NULL, 0);

	int timeBetweenActions = (rand() % 1000000 + 1);
	struct time actionTime;
	actionTime.seconds = ptr->time.seconds;
	actionTime.nanoseconds = ptr->time.nanoseconds;
	//addClock(&actionTime, 0, timeBetweenActions);

	srand(getpid());

	int pid = atoi(argv[0]);

	while(1) 
	{
	
		if((ptr->time.seconds > actionTime.seconds) || (ptr->time.seconds == actionTime.seconds && ptr->time.nanoseconds >= actionTime.nanoseconds))
		{
			actionTime.seconds = ptr->time.seconds;
			actionTime.nanoseconds = ptr->time.nanoseconds;
			//addClock(&actionTime, 0, timeBetweenActions);

			int chanceToRequest = rand() % (100 + 1 - 1) + 1;

			if(chanceToRequest < 70) 
			{
				ptr->resourceStruct.requestF = 1;
				int resourceIndex = rand() % (19 + 0 - 0) + 0;
				ptr->resourceStruct.index = resourceIndex;

			} 
			else 
			{
				ptr->resourceStruct.releaseF = 1;
			}
		}
	int chanceToTerminate = rand() % (100 + 1 - 1) + 1;

		if(chanceToTerminate < 10) 
		{	
			ptr->resourceStruct.termF = 1;
		}
		exit(0);

	}	
	return 0;
}

void addClock(struct time* time, int sec, int ns)
{
	time->seconds += sec;
	time->nanoseconds += ns;
	while(time->nanoseconds >= 1000000000){
		time->nanoseconds -=1000000000;
		time->seconds++;
	}
}
