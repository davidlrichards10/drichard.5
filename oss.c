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
sem_t *sem;

int requestGranted = 0;
int deadlockTermination = 0;
int normalTermination = 0;
int numDeadlockRan = 0;
int deadLockCheck = 1;
float average = 0;

int sharedResources[4];
int blockedQueue[20];
int resourceIndexQueue[20];
static int messageQueueId;
int stillActive[20];
int pidNum = 0;
int termed = 0;	
int checkBlocked(int pid, int result);
void release(int pid, int dl);
void deadlockAlgo();
void allocated(int pid, int resourceIndex);
void detach();
void sigErrors();
void incClock(struct time* time, int sec, int ns);
void rundeadlock();
void releasedl(int pid, int dl);
void printStats();

int timer = 10;
int blockPtr = 0;
int granted = 0;
int verbose = 0;
int lineCounter = 0;

char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) 
{
	int c;
	while((c=getopt(argc, argv, "v:i:t:h"))!= EOF)
	{
		switch(c)
		{
			case 'h':
				printf("\nInvocation: oss [-h] [-i x -v x -t x]\n");
                		printf("----------------------------------------------Program Options-------------------------------------------\n");
                		printf("       -h             Describe how the project should be run and then, terminate\n");
				printf("       -i             Type file name to print program information in (Default of log)\n");
				printf("       -v             Indicate whether you want verbose on/off by typing [-v 1] for on (Default off)\n");
				printf("       -t             Indicate timer amount (Default of 10 seconds)\n");				
				exit(0);
				break;
			case 'v':
				verbose = 1;
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

	if (verbose == 1)
	{
		printf("Verbose ON\n");
	}
	else
	{
		printf("Verbose OFF\n");
	}

	fp = fopen(outputFileName, "w");
	
	int maxPro = 100;
	srand(time(NULL));
	int count = 0;	

	if ((shmid = shmget(9784, sizeof(SharedMemory), IPC_CREAT | 0600)) < 0) 
	{
        	perror("Error: shmget");
        	exit(errno);
	}

	sem = sem_open("p5sem", O_CREAT, 0777, 1);	
  
	ptr = shmat(shmid, NULL, 0);

	pid_t cpid;

	int pid = 0;

 	time_t t;
	srand((unsigned) time(&t));
	int totalCount = 0;

	int i = 0;
	for(i=0; i <20; i++)
	{
		ptr->resourceStruct.max[i] = rand() % (10 + 1 - 1) + 1;
	}
	for(i=0; i < 4; i++){
		sharedResources[i] = rand() % (19 + 0 - 0) + 0;
	}
	for(i=0; i <20; i++)
	{
		ptr->resourceStruct.available[i] = ptr->resourceStruct.max[i];
	}
	int j = 0;
	for(j=0; j < 18; j++) 
	{
		for(i = 0; i < 20; i++) 
		{	
			ptr->descriptor[j].allocated[i] = 0;
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
	//struct time deadLockCheck;
	//deadLockCheck.seconds = 1;
	
	alarm(timer);
	while(totalCount < maxPro || count > 0)
	{ 							
		
			if(waitpid(cpid,NULL, WNOHANG)> 0)
			{
				count--;
			}

			incClock(&ptr->time,0,70000);
			int nextFork = (rand() % (500000000 - 1000000 + 1)) + 1000000;
			incClock(&randFork,0,nextFork);
			
			if(count < 18 && ptr->time.nanoseconds < nextFork) 
			{
						sem_wait(sem);
						randFork.seconds = ptr->time.seconds;
						randFork.nanoseconds = ptr->time.nanoseconds;
						sem_post(sem);
						nextFork = (rand() % (500000000 - 1000000 + 1)) + 1000000;
						incClock(&randFork,0,nextFork);			
						int l;
        					for(l=0; l<18;l++)
						{
                					if(stillActive[l] == -1)
							{
                        					termed++;
                					}
        					}

        					if(termed == 18)
						{	
							detach();
                					printStats();
							return 0;

                				} 
						else 
						{
                        				termed = 0;
                				}

        					if(stillActive[pidNum] != -1)
						{
                					pid = stillActive[pidNum];
                				} 
						else 
						{
                					int s = pidNum;
                					for(s=pidNum; s<18;s++)
							{
                						if(stillActive[s] == -1)
								{
                        						pidNum++;
                						} 
								else 
								{
                							break;
                						}

                					}

                					pid = stillActive[pidNum];

                				}
	
						cpid=fork();

						totalCount++;
						count++;
		
						if(cpid == 0) 
						{
							char passPid[10];
							sprintf(passPid, "%d", pid);		
							execl("./user","user", NULL);
							exit(0);
						}

						if(ptr->resourceStruct.requestF == 1)
						{
							ptr->resourceStruct.requestF = 0;
							int resourceIndex = ptr->resourceStruct.index;
							ptr->descriptor[pid].request[resourceIndex] = rand() % (10 + 1 - 1) + 1;
							
							if(verbose == 1 && lineCounter < 10000)
							{
								fprintf(fp,"Master has detected Process P%d requesting R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
								lineCounter++;
							}
							int resultBlocked = checkBlocked(pid,resourceIndex);

							if(resultBlocked == 0)
							{
								if(verbose == 1 && lineCounter < 10000)
								{
									fprintf(fp,"Master putting P%d in wait for R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
									lineCounter++;
								}
								int f, duplicate = 0;
								
								for(f=0; f< 18; f++)
								{
									if(blockedQueue[f] == pid)
									{
										duplicate++;
									}
								}
								
								if(duplicate == 0)
								{
									blockedQueue[blockPtr] = pid;
									resourceIndexQueue[blockPtr] = resourceIndex;
								} 
								else 
								{
									duplicate = 0;

								}
								blockPtr++;					
							} 
							else 
							{
								allocated(pid, resourceIndex);
								if(verbose == 1 && lineCounter < 10000)
								{
									fprintf(fp,"Master granting P%d  request R%d at time %d:%d\n",pid, resourceIndex, ptr->time.seconds,ptr->time.nanoseconds);
									lineCounter++;
								}
								requestGranted++;
								granted++;
								if( granted == 20  && verbose == 1 && lineCounter < 10000)
								{
									fprintf(fp,"\n\nCurrent Resource Table\n");
									granted = 0;
									int p = 0;
        								int j;
        								int i;
									fprintf(fp,"   ");
									for(j=1; j <= 20; j++)
									{
										fprintf(fp,"R%d  ", j);
									}
									fprintf(fp,"\n");
        								for(j =0; j <18; j++)
									{
										if(stillActive[j] != -1)
										{
                									p = j;
                        								fprintf(fp,"P%d   ",p);
                								for(i = 0; i < 20; i++) 
										{
                                							fprintf(fp,"%d   ", ptr->descriptor[p].allocated[i]);

                        							}
                        								fprintf(fp,"\n");
        									}
									}
									fprintf(fp,"\n");	
									lineCounter+=18;
								}	
							}

						}

						if(ptr->resourceStruct.termF == 1)
						{
							//deadLockCheck.seconds++;
							ptr->resourceStruct.termF = 0;
							if(verbose == 1 && lineCounter < 10000)
							{
								fprintf(fp,"Master terminating P%d at %d:%d\n",pid, ptr->time.seconds,ptr->time.nanoseconds);
								lineCounter++;
							}
							normalTermination++;
							stillActive[pid] = -1;

							release(pid,0);
						}

						if(ptr->resourceStruct.releaseF == 1)
						{
							ptr->resourceStruct.releaseF = 0;
							release(pid,0);
						}
						if((verbose == 1 || verbose == 0) && lineCounter < 10000)
						{
							rundeadlock();
							lineCounter+=20;
						}
			}
		}
	detach();
	return 0;
}

void printStats()
{
	fprintf(fp,"\nTotal Number of request granted = %d\n", requestGranted);
	fprintf(fp,"Total processes terminated in deadlock algo = %d\n", deadlockTermination);	
	fprintf(fp,"Total processes terminated normally = %d\n", normalTermination);
	fprintf(fp,"Total times deadlock algo ran = %d\n", numDeadlockRan);
	fprintf(fp,"Percent of process terminated in a deadlock on average = %d\n",average);

	printf("\nTotal Number of request granted = %d\n", requestGranted);
        printf("Total processes terminated in deadlock algo = %d\n", deadlockTermination);
        printf("Total processes terminated normally = %d\n", normalTermination);
        printf("Total times deadlock algo ran = %d\n", numDeadlockRan);
	printf("Percent of process terminated in a deadlock on average = %.3f%\n\n", (average / numDeadlockRan) * 100);
}

void rundeadlock()
{
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
	//printf("%d\n",ptr->time.seconds);
	//if(ptr->time.seconds == deadLockCheck)
	//{
		if(w > 0 &&  ptr->time.seconds == deadLockCheck)
		{
			deadLockCheck++;
			deadlockAlgo();
			numDeadlockRan++;
		}
	//}
		pidNum = 0;		
							
		int i = 0;
		for(i = 0; i <20; i++)
		{
			blockedQueue[i] = -1;
		}		
		blockPtr = 0;
						
	}	
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

void detach()
{
	shmctl(shmid, IPC_RMID, NULL);	
	sem_unlink("p5sem");
	sem_close(sem);
}

/* Function to control two types of interupts */
void sigErrors(int signum)
{
        if (signum == SIGINT)
        {
		printf("\nInterupted by ctrl-c\n");
        	printStats();
	}
        else
        {
                printf("\nInterupted by %d second alarm\n", timer);
        	printStats();
	}
	
	detach();
	printStats();
        exit(0);
}

void release(int pid, int dl)
{
	int i = 0, j = 0;
	int validationArray[20];
	int numberRes = 0;
	
	for(i=0; i < 20; i++) 
	{
		validationArray[i] = 0;	
	}
		if(verbose == 1)
		{
			fprintf(fp,"Master releasing P%d, Resources are: ",pid);
		}
	for(i=0; i < 20; i++) 
	{
		if(ptr->descriptor[pid].allocated[i] > 0)
		{		
			validationArray[i] = i;
			if(verbose == 1)
			{
				fprintf(fp,"R%d:%d ",i, ptr->descriptor[pid].allocated[i]);	
			}
			j++;
		} 
		else if(ptr->descriptor[pid].allocated[i] == 0)
		{
			numberRes++;
		}
	}
	
	if(numberRes == 20 && verbose == 1)
	{
		fprintf(fp," \n");
	} 
	else 
	{
		if(verbose == 1)
		{
			fprintf(fp,"\n");
		}
		int i;

		for(i=0; i < 20; i++) 
		{
			if(i == sharedResources[0] || i == sharedResources[1] || i == sharedResources[2] || i == sharedResources[3])
			{
				ptr->descriptor[pid].allocated[i] = 0;
			} 
			else 
			{
				ptr->resourceStruct.available[i] += ptr->descriptor[pid].allocated[i];
				ptr->descriptor[pid].allocated[i] = 0;			
			}
		}
	}
}

void releasedl(int pid, int dl)
{
        int i = 0, j = 0;
        int validationArray[20];
        int numberRes = 0;

        for(i=0; i < 20; i++)
        {
                validationArray[i] = 0;
        }
                if(verbose == 0)
                {
                        fprintf(fp,"Master releasing P%d, Resources are: ",pid);
                }
        for(i=0; i < 20; i++)
        {
                if(ptr->descriptor[pid].allocated[i] > 0)
                {
                        validationArray[i] = i;
                        if(verbose == 0)
                        {
                                fprintf(fp,"R%d:%d ",i, ptr->descriptor[pid].allocated[i]);
                        }
                        j++;
                }
                else if(ptr->descriptor[pid].allocated[i] == 0)
                {
                        numberRes++;
                }
        }

        if(numberRes == 20 && verbose == 0)
        {
                fprintf(fp," \n");
        }
        else
        {
                if(verbose == 0)
                {
                        fprintf(fp,"\n");
                }
                int i;

                for(i=0; i < 20; i++)
                {
                        if(i == sharedResources[0] || i == sharedResources[1] || i == sharedResources[2] || i == sharedResources[3])
                        {
                                ptr->descriptor[pid].allocated[i] = 0;
                        }
                        else
                        {
                                ptr->resourceStruct.available[i] += ptr->descriptor[pid].allocated[i];
                                ptr->descriptor[pid].allocated[i] = 0;
                        }
                }
        }
}

void deadlockAlgo() 
{
	fprintf(fp,"\nCurrent system resources\n");
	fprintf(fp,"Master running deadlock detection at time %d:%d\n", ptr->time.seconds,ptr->time.nanoseconds);
	int i = 0;
	float blockCount = 0;	
	float terminated = 0;
	float averageDL = 0;

	for(i =0; i < 20; i++)
	{
		if(blockedQueue[i] != -1)
		{
			blockCount++;
		}
	}
	fprintf(fp,"	Process ");
	for(i =0; i < blockCount; i++)
	{
		fprintf(fp, "P%d, ", blockedQueue[i]);

	}	
	fprintf(fp,"deadlocked\n");
	fprintf(fp,"	Attempting to resolve deadlock...\n");
	
	for(i =0; i < blockCount; i++)
	{	
		if(ptr->resourceStruct.available[resourceIndexQueue[i]] <= ptr->descriptor[blockedQueue[i]].request[resourceIndexQueue[i]] )
		{
			//terminated++;
			//average+=terminated;
			fprintf(fp,"	Killing process P%d\n", blockedQueue[i]);
			fprintf(fp,"		");
			//stillActive[i] = -1;
			//ptr->resourceStruct.termF == 1;
			deadlockTermination++;
			releasedl(blockedQueue[i],1);		
			
			if(i+1 < blockCount)
			{
				fprintf(fp,"	Master running deadlock detection after P%d killed\n",blockedQueue[i]);
				fprintf(fp,"	Processes ");
				terminated++;
				int m;
				for(m=i+1; m <blockCount; m++)
				{
					fprintf(fp,"P%d, ",blockedQueue[m]);	
				}
				fprintf(fp,"deadlocked\n");
			}

		} 
		else 
		{
			//grant++;
			allocated(blockedQueue[i], resourceIndexQueue[i]);
			fprintf(fp,"	Master granting P%d request R%d at time %d:%d\n",blockedQueue[i], resourceIndexQueue[i], ptr->time.seconds,ptr->time.nanoseconds);

		}
	}	
	fprintf(fp,"System is no longer in deadlock\n");
	fprintf(fp,"\n");
	averageDL = terminated / blockCount;
	average += averageDL; 
}

int checkBlocked(int pid, int resourceIndex) 
{
	if(ptr->resourceStruct.available[resourceIndex] > 0)
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
		ptr->resourceStruct.available[resourceIndex] = ptr->resourceStruct.max[resourceIndex];
	} 
	else 
	{
		ptr->resourceStruct.available[resourceIndex] = ptr->resourceStruct.available[resourceIndex] - ptr->descriptor[pid].request[resourceIndex];
	}
	ptr->descriptor[pid].allocated[resourceIndex] = ptr->descriptor[pid].request[resourceIndex];
}
