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
#include "shared.h"

int shmid; 
SharedMemory* ptr;

int sharedResources[4];
int blockedQueue[20];
int resourceIndexQueue[20];
static int messageQueueId;
int stillActive[20];
int pidNum = 0;
int termed = 0;	
int checkBlocked(int pid, int result);
void release(int pid, int dl);
void checkDeadLockDetection();
void allocated(int pid, int resourceIndex);
void detach();
void sigErrors();
void addClock(struct time* time, int sec, int ns);

char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) {
	int c;
	int maxPro;
	int v = 1;
	int timer = 10;
	while((c=getopt(argc, argv, "v:i:t:h"))!= EOF)
	{
		switch(c)
		{
			case 'h':
				printf("\nInvocation: oss [-h] [-i x -v x -t x]\n");
                		printf("------------------------------------------Program Options-----------------------------------\n");
                		printf("       -h             Describe how the project should be run and then, terminate\n");
				printf("       -i             Type file name to print program information in (Default of log)\n");
				printf("       -v             Indicate whether you want verbose on/off by typing 1 or 0 (Default on)\n");
				printf("       -t             Indicate timer amount (Default of 10 seconds)\n");				
				exit(0);
				break;
			case 'v':
				v = atoi(optarg);
				break;
			case 'i':
                		strcpy(outputFileName, optarg);
                		break;
			case 't':
				timer = atoi(optarg);
				break;
			default:
				return -1;
		}
	}

	int verbose = v;
	fp = fopen(outputFileName, "w");
	
	maxPro = 100;
	srand(time(NULL));
	int count = 0;
	

	if ((shmid = shmget(9784, sizeof(SharedMemory), IPC_CREAT | 0600)) < 0) {
        	perror("Error: shmget");
        	exit(errno);
	}
  
	ptr = shmat(shmid, NULL, 0);

	pid_t cpid;

	int pid = 0;

 	time_t t;
	srand((unsigned) time(&t));
	int totalCount = 0;
	int blockPtr = 0;

	int i = 0;
	for(i=0; i <20; i++)
	{
		ptr->resources.max[i] = rand() % (10 + 1 - 1) + 1;
	}
	for(i=0; i < 4; i++){
		sharedResources[i] = rand() % (19 + 0 - 0) + 0;
	}
	for(i=0; i <20; i++)
	{
		ptr->resources.available[i] = ptr->resources.max[i];
	}
	int j = 0;
	for(j=0; j < 18; j++) 
	{
		for(i = 0; i < 20; i++) 
		{	
			ptr->resourceDescriptor[j].allocated[i] = 0;
		}

	}
	for(i = 0; i <20; i++)
	{
		blockedQueue[i] = -1;
	}		
	for(i = 0; i < 18; i++)
	{
		stillActive[i] = i;
	}

	/* Catch ctrl-c and 3 second alarm interupts */
	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on ctrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding 3 second alarm
        {
                exit(0);
        }

	struct time randFork;
	struct time deadLockCheck;
	deadLockCheck.seconds = 1;
	
	alarm(timer);
	while(totalCount < maxPro || count > 0){ 							
		
			if(waitpid(cpid,NULL, WNOHANG)> 0){
				count--;
			}

			addClock(&ptr->time,0,190000);
			int nextFork = (rand() % (500000000 - 1000000 + 1)) + 1000000;
			addClock(&randFork,0,nextFork);
			
			if(count < 18 && ptr->time.nanoseconds < nextFork) 
			{			
						int l;	
						for(l=0; l<18;l++){
							if(stillActive[l] == -1){
								termed++;					
							} 
						}
						
						if(termed == 18){					
							detach();
 							return 0;

						} else {
							termed = 0;
						}

						if(stillActive[pidNum] != -1){
							pid = stillActive[pidNum];
						} else {	
							
							int s = pidNum;
							for(s=pidNum; s<18;s++){
								if(stillActive[s] == -1){
									pidNum++;
								} else {
									break;
								}

							}
							
							pid = stillActive[pidNum];

						}
	
						cpid=fork();

						totalCount++;
						count++;
		
						if(cpid < 0) {
							perror("Fork failed");
						} else if(cpid == 0) {		
							execl("./user", "user",NULL);
							exit(0);
						} else {
			
						}

						if(ptr->resources.requestF == 1)//strcmp(message.mtext, "Request") == 0 )
						{
							ptr->resources.requestF = 0;
							int resourceIndex = rand() % (19 + 0 - 0) + 0;
							ptr->resourceDescriptor[pid].request[resourceIndex] = rand() % (10 + 1 - 1) + 1;
							fprintf(fp,"Master has detected Process P%d requesting R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
							
							int resultBlocked = checkBlocked(pid,resourceIndex);

							if(resultBlocked == 0){

									fprintf(fp,"Master blocking P%d requesting R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
								
								int f, duplicate = 0;
								for(f=0; f< 18; f++){
									if(blockedQueue[f] == pid){
										duplicate++;
									}
								}
								
								if(duplicate == 0){
									blockedQueue[blockPtr] = pid;
									resourceIndexQueue[blockPtr] = resourceIndex;
								} else {
									duplicate = 0;
								}
								blockPtr++;					

							} else {
								allocated(pid, resourceIndex);

									fprintf(fp,"Master granting P%d  request R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
								
						}	
						}

						if(ptr->resources.termF == 1)//strcmp(message.mtext, "Terminated") == 0 )
						{
							ptr->resources.termF = 0;
							//trackProcessTerminated++;
							fprintf(fp,"Master terminating P%d at %d:%d\n",pid, ptr->time.seconds,ptr->time.nanoseconds);
							stillActive[pid] = -1;

							release(pid,0);
						}

						if(ptr->resources.releaseF == 1)//strcmp(message.mtext, "Release") == 0 )
						{
							ptr->resources.releaseF = 0;
							release(pid,0);
						}

						if(pidNum < 17)
						{			
							pidNum++;	
						} 
						else 
						{
						
							int k,w=0;
							for(k =0; k < 20; k++)
							{
								if(blockedQueue[k] != -1)
								{
									w++;
								}
							}

							if(w > 0 )//&& ptr->time.seconds == deadLockCheck.seconds)
							{
								deadLockCheck.seconds++;
                                                                checkDeadLockDetection();
							}

							pidNum = 0;		
							
							int i = 0;
							for(i = 0; i <20; i++)
							{
								blockedQueue[i] = -1;
							}		
							blockPtr = 0;
						
						}

					//}
			}
		}
	detach();
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

void detach()
{
	//sem_unlink("clocksem");
	msgctl(messageQueueId,IPC_RMID,NULL);
	shmctl(shmid, IPC_RMID, NULL);	
}

/* Function to control two types of interupts */
void sigErrors(int signum)
{
        if (signum == SIGINT)
        {
		printf("\nInterupted by ctrl-c\n");
        }
        else
        {
                printf("\nInterupted by 3 second alarm\n");
        }
	
	detach();
        exit(0);
}

void release(int pid, int dl)
{
	int i = 0, j = 0;
	int validationArray[20];
	int returnResult = 0;
	
	for(i=0; i < 20; i++) 
	{
		validationArray[i] = 0;	
	}
		fprintf(fp,"Master releasing P%d, Resources are: ",pid);

	for(i=0; i < 20; i++) 
	{
		if(ptr->resourceDescriptor[pid].allocated[i] > 0)
		{		
			validationArray[i] = i;
			fprintf(fp,"R%d:%d ",i, ptr->resourceDescriptor[pid].allocated[i]);	
			j++;
		} 
		else if(ptr->resourceDescriptor[pid].allocated[i] == 0)
		{
			returnResult++;
		}
	}
	
	if(returnResult == 20)
	{
		fprintf(fp,"None\n");
	} 
	else 
	{
		fprintf(fp,"\n");
		int i;

		for(i=0; i < 20; i++) 
		{
			if(i == sharedResources[0] || i == sharedResources[1] || i == sharedResources[2] || i == sharedResources[3])
			{
				ptr->resourceDescriptor[pid].allocated[i] = 0;
			} 
			else 
			{
				ptr->resources.available[i] += ptr->resourceDescriptor[pid].allocated[i];
				ptr->resourceDescriptor[pid].allocated[i] = 0;			
			}
		}
	}
}

void checkDeadLockDetection() 
{
	//timesDeadlockRun++;
	fprintf(fp,"\nCurrent system resources\n");
	fprintf(fp,"Master running deadlock detection at time %d:%d\n", ptr->time.seconds,ptr->time.nanoseconds);
	int i = 0;
	int manyBlock = 0;	
	int average = 0;
	for(i =0; i < 20; i++)
	{
		if(blockedQueue[i] != -1)
		{
			manyBlock++;
		}
	}
	fprintf(fp,"	Process ");
	for(i =0; i < manyBlock; i++)
	{
		fprintf(fp, "P%d, ", blockedQueue[i]);

	}	
	fprintf(fp,"deadlocked\n");
	fprintf(fp,"	Attempting to resolve deadlock...\n");
	
	for(i =0; i < manyBlock; i++)
	{	
		if(ptr->resources.available[resourceIndexQueue[i]] <= ptr->resourceDescriptor[blockedQueue[i]].request[resourceIndexQueue[i]] )
		{
			fprintf(fp,"	Killing process P%d\n", blockedQueue[i]);
			fprintf(fp,"		");
			release(blockedQueue[i],1);		
			
			if(i+1 < manyBlock)
			{
				fprintf(fp,"	Master running deadlock detection after P%d killed\n",blockedQueue[i]);
				fprintf(fp,"	Processes ");
				//numTerminatedDeadLock++;
				int m;
				for(m=i+1; m <manyBlock; m++)
				{
					fprintf(fp,"P%d, ",blockedQueue[m]);	
				}
				fprintf(fp,"deadlocked\n");
			}

		} 
		else 
		{
			allocated(blockedQueue[i], resourceIndexQueue[i]);
			fprintf(fp,"	Master granting P%d request R%d at time %d:%d\n",blockedQueue[i], resourceIndexQueue[i], ptr->time.seconds,ptr->time.nanoseconds);

		}
	}	
	fprintf(fp,"System is no longer in deadlock\n");
	fprintf(fp,"\n");
}

int checkBlocked(int pid, int resourceIndex) 
{
	if(ptr->resources.available[resourceIndex] > 0)//= ptr->resourceDescriptor[pid].request[resourceIndex] )
	{
		return 1;
	} 
	else 
	{
		return 0;
	}
}


void allocated(int pid, int resourceIndex) 
{
	if(resourceIndex == sharedResources[0] || resourceIndex == sharedResources[1] || resourceIndex == sharedResources[2] || resourceIndex == sharedResources[3])
	{
		ptr->resources.available[resourceIndex] = ptr->resources.max[resourceIndex];
	} 
	else 
	{
		ptr->resources.available[resourceIndex] = ptr->resources.available[resourceIndex] - ptr->resourceDescriptor[pid].request[resourceIndex];
	}
	ptr->resourceDescriptor[pid].allocated[resourceIndex] = ptr->resourceDescriptor[pid].request[resourceIndex];
}


