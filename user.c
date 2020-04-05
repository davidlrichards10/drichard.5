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

typedef struct message {
    long myType;
    char mtext[512];
} Message;

static int messageQueueId;
int terminate = 0;

int main(int argc, char* argv[]) {

	Message message;	

     	if ((shmid = shmget(9784, sizeof(SharedMemory), 0600)) < 0) {
            perror("Error: shmget");
            exit(errno);
     	}
   
    	 if ((messageQueueId = msgget(3000, 0644)) == -1) {
            perror("Error: msgget");
            exit(errno);
      	}


 	 
	shmPtr = shmat(shmid, NULL, 0);

	srand(getpid());

	while(1) {
	
		if (msgrcv(messageQueueId, &message,sizeof(message)+1,1,0) == -1) {
			perror("msgrcv");
		}

		int chance = rand() % (100 + 1 - 1) + 1;
		

		if(chance > 0 && chance < 63) {
			strcpy(message.mtext,"Request");


		} else if(chance > 62 && chance < 93) {
			strcpy(message.mtext,"Release");

		} else if(chance > 92 && chance  < 101) {	
			strcpy(message.mtext,"Terminated");
			terminate = 1;
		}
	
		message.myType = 2;	
			
		if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
			perror("msgsnd");
			exit(1);
		} 

		exit(0);
	

	}	
	return 0;
}
