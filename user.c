/* Author: David Richards
 * Date: Tue April 14th
 * Assignment: CS4760 hw5
 * File: user.c
 */

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
#include "shared.h"

/* for shared memory and semaphore */
int shmid; 
sm* ptr;
sem_t *sem;

void incClock(struct time* time, int sec, int ns);

int main(int argc, char* argv[]) 
{
	/* get shared memory */
     	if ((shmid = shmget(9784, sizeof(sm), 0600)) < 0) 
	{
            perror("Error: shmget");
            exit(errno);
     	}
	
	/*open semaphore used to protect the clock */
	sem = sem_open("p5sem", 0);   

	/* attach to shared memory */
	ptr = shmat(shmid, NULL, 0);

	/* set time for when a process should either request or release */
	int nextMove = (rand() % 1000000 + 1);
	struct time moveTime;
	sem_wait(sem);
	moveTime.seconds = ptr->time.seconds;
	moveTime.nanoseconds = ptr->time.nanoseconds;
	sem_post(sem);

	/* set time for a when a process should check if its time to terminate */
	int termination = (rand() % (250 * 1000000) + 1);
	struct time termCheck;
	sem_wait(sem);
	termCheck.seconds = ptr->time.seconds;
	termCheck.nanoseconds = ptr->time.nanoseconds;
	sem_post(sem);

	time_t t;
	time(&t);	
	srand((int)time(&t) % getpid());

	while(1) 
	{
		/* if its time for the next action, check whether to request or release */
		if((ptr->time.seconds > moveTime.seconds) || (ptr->time.seconds == moveTime.seconds && ptr->time.nanoseconds >= moveTime.nanoseconds))
		{
			/* set time for next acion check */
			sem_wait(sem);
			moveTime.seconds = ptr->time.seconds;
			moveTime.nanoseconds = ptr->time.nanoseconds;
			sem_post(sem);
			incClock(&moveTime, 0, nextMove);

			int chanceToRequest = rand() % (100 + 1 - 1) + 1;

			/* process has a 95% chance to request, otherwise release and change shared memory flags accordingly*/
			if(chanceToRequest < 95) 
			{
				/* if a process is to request something, put that request into shared memory */
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

		/* see if its time to check for termination */
		if((ptr->time.seconds > termCheck.seconds) || (ptr->time.seconds == termCheck.seconds && ptr->time.nanoseconds >= termCheck.nanoseconds))
		{
			/* set time for next termination check */
			termination = (rand() % (250 * 1000000) + 1);
			termCheck.seconds = ptr->time.seconds;
			termCheck.nanoseconds = ptr->time.nanoseconds;
			incClock(&termCheck, 0, termination);
			
			/* process has 10% to terminate if it is time */
			if(chanceToTerminate < 10) 
			{	
				ptr->resourceStruct.termF = 1;
			}
			exit(0);
		}
	}	
	return 0;
}

/* function to increment the clock and is protected via semaphore */
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
