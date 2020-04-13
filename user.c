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

SharedMemory* ptr;
sem_t *sem;

void incClock(struct time* time, int sec, int ns);

int main(int argc, char* argv[]) 
{
     	if ((shmid = shmget(9784, sizeof(SharedMemory), 0600)) < 0) 
	{
            perror("Error: shmget");
            exit(errno);
     	}
	
	sem = sem_open("p5sem", 0);   

	ptr = shmat(shmid, NULL, 0);

	int nextMove = (rand() % 1000000 + 1);
	struct time moveTime;
	sem_wait(sem);
	moveTime.seconds = ptr->time.seconds;
	moveTime.nanoseconds = ptr->time.nanoseconds;
	sem_post(sem);
	//incClock(&moveTime, 0, nextMove);

	int termination = (rand() % (250 * 1000000) + 1);
	struct time termCheck;
	termCheck.seconds = ptr->time.seconds;
	termCheck.nanoseconds = ptr->time.nanoseconds;
	//incClock(&termCheck, 0, termination);

	//srand(getpid());

	time_t t;
	time(&t);	
	srand((int)time(&t) % getpid());

	int pid = atoi(argv[0]);

	while(1) 
	{
	
		if((ptr->time.seconds > moveTime.seconds) || (ptr->time.seconds == moveTime.seconds && ptr->time.nanoseconds >= moveTime.nanoseconds))
		{
			sem_wait(sem);
			moveTime.seconds = ptr->time.seconds;
			moveTime.nanoseconds = ptr->time.nanoseconds;
			sem_post(sem);
			incClock(&moveTime, 0, nextMove);

			int chanceToRequest = rand() % (100 + 1 - 1) + 1;

			if(chanceToRequest < 95) 
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

		if((ptr->time.seconds > termCheck.seconds) || (ptr->time.seconds == termCheck.seconds && ptr->time.nanoseconds >= termCheck.nanoseconds))
		{
			termination = (rand() % (250 * 1000000) + 1);
			termCheck.seconds = ptr->time.seconds;
			termCheck.nanoseconds = ptr->time.nanoseconds;
			incClock(&termCheck, 0, termination);
			
			if(chanceToTerminate < 10) 
			{	
				ptr->resourceStruct.termF = 1;
			}
			exit(0);
		}
	}	
	return 0;
}

void incClock(struct time* time, int sec, int ns)
{
	sem_wait(sem);
	time->seconds += sec;
	time->nanoseconds += ns;
	while(time->nanoseconds >= 1000000000)
	{
		time->nanoseconds -=1000000000;
		time->seconds++;
	}
	sem_post(sem);
}
