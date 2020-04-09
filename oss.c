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

typedef struct message {
    long myType;
    char mtext[512];
} Message;


void signalCall(int signum);

int shmid; 
SharedMemory* shmPtr;
Clock launchTime;

int shareable[4];
int queueArray[20];
int resultArray[20];
static int messageQueueId;
int times;
int availableActive = 0;
int randomresources();
int randomInterval();
int randomizeShareablePosition();
int nonTerminated[20];
int num = 0;
int terminatedNumber = 0;	
void signalCall(int signum);
void userProcess();
int ifBlockResources(int fakePid, int result);
void release(int fakePid, int dl);
void checkDeadLockDetection();
void addRequestToAllocated(int fakePid, int results);
void detach();
void sigErrors();
int generateRequest(int fakePid);

char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) {
	int c;
	int requestNumbers = 0;
	int maxChildProcess;
	int v = 1;
	int timer = 10;
	while((c=getopt(argc, argv, "v:i:t:h"))!= EOF)
	{
		switch(c){
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
	
	maxChildProcess = 100;
	int bufSize = 200;
	char errorMessage[bufSize];
	Message message;


	int ptr_count = 0;
	

	if ((shmid = shmget(9784, sizeof(SharedMemory), IPC_CREAT | 0600)) < 0) {
        	perror("Error: shmget");
        	exit(errno);
	}
  
 
	if ((messageQueueId = msgget(3000, IPC_CREAT | 0644)) == -1) {
        	perror("Error: mssget");
       		 exit(errno);
    	}
  
	 shmPtr = shmat(shmid, NULL, 0);
  	 shmPtr->clockInfo.seconds = 0; 
   	 shmPtr->clockInfo.nanoSeconds = 0;  

	pid_t childpid;

	int fakePid = 0;
	shmPtr->clockInfo.nanoSeconds = 0;
	shmPtr->clockInfo.seconds = 0;

	//srand(NULL);
 	time_t t;
	srand((unsigned) time(&t));
	int totalCount = 0;
	int blockPos = 0;

	int i = 0;
	for(i=0; i <20; i++)
	{
		shmPtr->resources.max[i] = randomResources();
	}
	for(i=0; i < 4; i++){
		shareable[i] = randomizeShareablePosition();
	}
	for(i=0; i <20; i++)
	{
		shmPtr->resources.available[i] = shmPtr->resources.max[i];
	}
	int j = 0;
	for(j=0; j < 18; j++) 
	{
		for(i = 0; i < 20; i++) 
		{	
			shmPtr->resourceDescriptor[j].allocated[i] = 0;
		}

	}
	for(i = 0; i <20; i++)
	{
		queueArray[i] = -1;
	}		
	for(i = 0; i < 18; i++)
	{
		nonTerminated[i] = i;
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

	alarm(timer);
	while(totalCount < maxChildProcess || ptr_count > 0){ 							
		
			if(waitpid(childpid,NULL, WNOHANG)> 0){
				ptr_count--;
			}


			if(ptr_count < 18 )
				{			
						int l;	
						
						for(l=0; l<18;l++){
							if(nonTerminated[l] == -1){
								terminatedNumber++;					
							} 
						}
						
						if(terminatedNumber == 18){					
							detach();
 							return 0;

						} else {
							terminatedNumber = 0;
						}

						if(nonTerminated[num] != -1){
							fakePid = nonTerminated[num];
						} else {	
							
							int s = num;
							for(s=num; s<18;s++){
								if(nonTerminated[s] == -1){
									num++;
								} else {
									break;
								}

							}
							
							fakePid = nonTerminated[num];

						}
	
						message.myType = 1;
						char buffer1[100];
						sprintf(buffer1, "%d", fakePid);
						strcpy(message.mtext,buffer1);	
			
						if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
							perror("msgsnd");
							exit(1);
						}
						
						childpid=fork();

						totalCount++;
						ptr_count++;
		
						if(childpid < 0) {
							perror("Fork failed");
						} else if(childpid == 0) {		
							execl("./user", "user",NULL);
							snprintf(errorMessage, sizeof(errorMessage), "%s: Error: ", argv[0]);
	    	 					perror(errorMessage);		
							exit(0);
						} else {
			
						}
			
						/*if (msgrcv(messageQueueId, &message,sizeof(message)+1,2,0) == -1) {
							perror("msgrcv");

						}*/	

						if(shmPtr->resources.requestF == 1)//strcmp(message.mtext, "Request") == 0 )
						{
							shmPtr->resources.requestF = 0;
							int results = generateRequest(fakePid);
							fprintf(fp,"Master has detected Process P%d requesting R%d at time %d:%d\n",fakePid, results,shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds );
							
							int resultBlocked = ifBlockResources(fakePid,results);

							if(resultBlocked == 0){

									fprintf(fp,"Master blocking P%d requesting R%d at time %d:%d\n",fakePid, results,shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds );
								
								int f, duplicate = 0;
								for(f=0; f< 18; f++){
									if(queueArray[f] == fakePid){
										duplicate++;
									}
								}
								
								if(duplicate == 0){
									queueArray[blockPos] = fakePid;
									resultArray[blockPos] = results;
								} else {
									duplicate = 0;
								}
								blockPos++;					

							} else {
								addRequestToAllocated(fakePid, results);

									fprintf(fp,"Master granting P%d  request R%d at time %d:%d\n",fakePid, results, shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds);
								if(requestNumbers == 20){
									requestNumbers = 0;
								requestNumbers++;
								//trackRequest++;
							}
						}	
						}

						if(shmPtr->resources.termF == 1)//strcmp(message.mtext, "Terminated") == 0 )
						{
							shmPtr->resources.termF = 0;
							//trackProcessTerminated++;
							fprintf(fp,"Master terminating P%d at %d:%d\n\n",fakePid, shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds);
							nonTerminated[fakePid] = -1;

							release(fakePid,0);
						}

						if(shmPtr->resources.releaseF == 1)//strcmp(message.mtext, "Release") == 0 )
						{
							shmPtr->resources.releaseF = 0;
							release(fakePid,0);
						}

					
						if(num < 17)
						{			
							num++;	
						} 
						else 
						{
						
							int k,w=0;
							for(k =0; k < 20; k++)
							{
								if(queueArray[k] != -1)
								{
									w++;
								}
							}

							if(w > 0)
							{
								checkDeadLockDetection();
							}

							num = 0;		
							//initializeQueueArray();
							int i = 0;
							for(i = 0; i <20; i++)
							{
								queueArray[i] = -1;
							}		
							blockPos = 0;
						}
			}
		}
	detach();
	return 0;
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

void release(int fakePid, int dl)
{
	int i = 0, j = 0;
	int validationArray[20];
	int returnResult = 0;
	
	for(i=0; i < 20; i++) 
	{
		validationArray[i] = 0;	
	}
		fprintf(fp,"Master releasing P%d, Resources are: ",fakePid);

	for(i=0; i < 20; i++) 
	{
		if(shmPtr->resourceDescriptor[fakePid].allocated[i] > 0)
		{		
			validationArray[i] = i;
			fprintf(fp,"R%d:%d ",i, shmPtr->resourceDescriptor[fakePid].allocated[i]);	
			j++;
		} 
		else if(shmPtr->resourceDescriptor[fakePid].allocated[i] == 0)
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
			if(i == shareable[0] || i == shareable[1] || i == shareable[2] || i == shareable[3])
			{
				shmPtr->resourceDescriptor[fakePid].allocated[i] = 0;
			} 
			else 
			{
				shmPtr->resources.available[i] += shmPtr->resourceDescriptor[fakePid].allocated[i];
				shmPtr->resourceDescriptor[fakePid].allocated[i] = 0;			
			}
		}
	}
}

void checkDeadLockDetection() 
{
	//timesDeadlockRun++;
	fprintf(fp,"\nCurrent system resources\n");
	fprintf(fp,"Master running deadlock detection at time %d:%d\n",shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
	int i = 0;
	int manyBlock = 0;	
	int average = 0;
	for(i =0; i < 20; i++)
	{
		if(queueArray[i] != -1)
		{
			manyBlock++;
		}
	}
	fprintf(fp,"	Process ");
	for(i =0; i < manyBlock; i++)
	{
		fprintf(fp, "P%d, ", queueArray[i]);

	}	
	fprintf(fp,"deadlocked\n");
	fprintf(fp,"	Attempting to resolve deadlock...\n");
	
	for(i =0; i < manyBlock; i++)
	{	
		if(shmPtr->resources.available[resultArray[i]] <= shmPtr->resourceDescriptor[queueArray[i]].request[resultArray[i]] )
		{
			fprintf(fp,"	Killing process P%d\n", queueArray[i]);
			fprintf(fp,"		");
			release(queueArray[i],1);		
			
			if(i+1 < manyBlock)
			{
				fprintf(fp,"	Master running deadlock detection after P%d killed\n",queueArray[i]);
				fprintf(fp,"	Processes ");
				//numTerminatedDeadLock++;
				int m;
				for(m=i+1; m <manyBlock; m++)
				{
					fprintf(fp,"P%d, ",queueArray[m]);	
				}
				fprintf(fp,"deadlocked\n");
			}

		} 
		else 
		{
			addRequestToAllocated(queueArray[i], resultArray[i]);
			fprintf(fp,"	Master granting P%d request R%d at time %d:%d\n",queueArray[i], resultArray[i], shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds);

		}
	}	
	
	//average = numTerminatedDeadLock / manyBlock;
	//averageDeadlock += average;
	
	fprintf(fp,"System is no longer in deadlock\n");
	fprintf(fp,"\n");
}

int ifBlockResources(int fakePid, int result) 
{
	if(shmPtr->resources.available[result] >= shmPtr->resourceDescriptor[fakePid].request[result] )
	{
		return 1;
	} 
	else 
	{
		return 0;
	}
}


void addRequestToAllocated(int fakePid, int results) 
{
	if(results == shareable[0] || results == shareable[1] || results == shareable[2] || results == shareable[3])
	{
		shmPtr->resources.available[results] = shmPtr->resources.max[results];
	} 
	else 
	{
		shmPtr->resources.available[results] = shmPtr->resources.available[results] - shmPtr->resourceDescriptor[fakePid].request[results];
	}
	shmPtr->resourceDescriptor[fakePid].allocated[results] = shmPtr->resourceDescriptor[fakePid].request[results];
}

int  generateRequest(int fakePid) 
{
	int resourcesLoc = rand() % (19 + 0 - 0) + 0;
	shmPtr->resourceDescriptor[fakePid].request[resourcesLoc] = rand() % (10 + 1 - 1) + 1;
	return resourcesLoc;
}

int  randomIntervalLaunch() 
{
	int times = 0; 
	times = rand() % (25000000 + 1 - 1) + 1;
	return times;
}

int  randomInterval() 
{
	int times = 0; 
	times = rand() % (50000000 + 1 - 1) + 1;
	return times;
}


int randomResources() 
{
	int resources = 0;
	resources = rand() % (10 + 1 - 1) + 1;
	return resources;
}


int randomizeShareablePosition() 
{
	int shareable = 0;
	shareable = rand() % (19 + 0 - 0) + 0;
	return shareable;
}


