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
SharedMemory* shmPtr;
void addClock(struct time* time, int sec, int ns);

typedef struct message {
    long myType;
    char mtext[512];
} Message;

static int messageQueueId;
int terminate = 0;

int main(int argc, char* argv[]) {

	/*int termination = (rand() % (250 * 1000000) + 1);
	struct time nextTerminationCheck;
        nextTerminationCheck.seconds = shmPtr->time.seconds;
        nextTerminationCheck.nanoseconds = shmPtr->time.nanoseconds;
        addClock(&nextTerminationCheck, 0, termination);
*/
	Message message;	

     	if ((shmid = shmget(9784, sizeof(SharedMemory), 0600)) < 0) {
            perror("Error: shmget");
            exit(errno);
     	}
   
    	/* if ((messageQueueId = msgget(3000, 0644)) == -1) {
            perror("Error: msgget");
            exit(errno);
      	}*/


 	 
	shmPtr = shmat(shmid, NULL, 0);

	srand(getpid());

	while(1) {
	
		/*if (msgrcv(messageQueueId, &message,sizeof(message)+1,1,0) == -1) {
			perror("msgrcv");
		}*/

		int chance = rand() % (100 + 1 - 1) + 1;
		int chance1 = rand() % (100 + 1 - 1) + 1;

		if(chance > 0 && chance < 63) {
			shmPtr->resources.requestF = 1;
			//strcpy(message.mtext,"Request");


		} else {//if(chance > 62 && chance < 93) {
			shmPtr->resources.releaseF = 1;
			//strcpy(message.mtext,"Release");

		}
		//if(shmPtr->time.seconds > 0)//nextTerminationCheck.seconds)
		//{
			/*termination = (rand() % (250 * 1000000) + 1);
			nextTerminationCheck.seconds = shmPtr->time.seconds;
			nextTerminationCheck.nanoseconds = shmPtr->time.nanoseconds;
			addClock(&nextTerminationCheck, 0, termination);
 */
			if(chance1 < 10) 
			{	
				shmPtr->resources.termF = 1;
				//strcpy(message.mtext,"Terminated");
				terminate = 1;
				//shmPtr->resources.termF = 1;
			}
		//}
	
		/*message.myType = 2;	
			
		if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
			perror("msgsnd");
			exit(1);
		} */

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
