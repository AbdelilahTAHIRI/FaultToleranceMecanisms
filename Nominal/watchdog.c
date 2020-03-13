/**
@Content: This file contains the WATCHDOG process implementation
**/

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include "watchdog.h"

//int clock_gettime(clockid_t clk_id, struct timespec *tp);
static int cout=0;
static int ServerCount=1;
//timespec struct to measure detection time.
static struct timespec ts1;
static struct timespec ts2;

int IncCount( ) {
cout += 1 ;
return ( cout ) ;
}

int GetCount( void ) {
return ( cout ) ;
}

//This function creates a server process with the specified mode
int CreateServer(int index)
{
	int pid;
	char str_index[4];
	pid=fork();
	switch(pid)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_index,"%d",index);
			char *args[]={"server",str_index,NULL};
			int ret;
			ret=execv("./bin/server",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}

	return pid;
}
//This function creates a killer process to kill specified process
int CreateKiller(int pid_to_kill)
{
	int pid;
	char str_pid_to_kill[4];
	pid=fork();
	switch(pid)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_pid_to_kill,"%d",pid_to_kill);
			char *args[]={"killer",str_pid_to_kill,NULL};
			int ret;
			ret=execv("./bin/killer",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}
	return pid;
}


static void sigHandler(int sig)
{
	int ret;
	if(sig==SIGUSR1)
		IncCount();
	else if(sig==SIGUSR2)
	{
		ret = clock_gettime (CLOCK_MONOTONIC, &ts1);
		if (ret)
			perror ("clock_gettime");
	}
}


int main( int argc, char * argv[])
{
	int memory_counter;
	pid_t pid_primary;
	pid_t pid_killer;
	char str_pid_primary[4];	
	int ret;
	int status;
	int fifo_fd;
	float delay;
	int i=0;
	
	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
	{
		perror("signal");
		exit(EXIT_FAILURE);
	}

	if(signal(SIGUSR2 ,sigHandler)==SIG_ERR) 
	{
		perror("signal");
		exit(EXIT_FAILURE);
	}
	
	fifo_fd=open("./tmp/fifo1", O_RDONLY);
	ServerCount=1;
	pid_primary= CreateServer(ServerCount);
	

	pid_killer=CreateKiller(pid_primary);
	
	while (1)
	{
		memory_counter=GetCount();
		//sleep(TIMEOUT);//Attendre le temps que le primaire donne un coup de pied au watchdog
		for(i=0;i<150;i++)
		{usleep(1);}
	
		if(memory_counter==GetCount())
		{
			//Get the time detection
			ret = clock_gettime (CLOCK_MONOTONIC, &ts2);
			if (ret) {perror ("clock_gettime");}
			
			//Compute the Delay Time
			delay=(ts2.tv_sec-ts1.tv_sec)+(ts2.tv_nsec-ts1.tv_nsec)*0.0000000001;
			
			printf("\033[1;33m Detection time is %f sec \n",delay);

			printf("\033[1;33m WATCHDOG: Server Process is dead\n");
			fflush(stdout);
			kill(pid_primary, SIGKILL); //tuer le primaire defaillant
			
			//The PRIMARY Still a Zombie, wait upon to terminate it.
			ret = waitpid (pid_primary,&status,WNOHANG);
			if(ret==-1)
				perror ("waitpid");
			if (WIFEXITED (status))
				printf ("Normal termination with exit status=%d\033[0m\n",WEXITSTATUS (status));
			if (WIFSIGNALED (status))
				printf ("Killed by signal=%d%s\033[0m\n",WTERMSIG (status),WCOREDUMP (status) ? " (dumped core)" : "");
			if (WIFSTOPPED (status))
				printf ("Stopped by signal=%d\033[0m\n",WSTOPSIG (status));
			if (WIFCONTINUED (status))
				printf ("Continued\033[0m\n");
			fflush(stdout);
			
			ServerCount++;
			pid_primary= CreateServer(ServerCount);
			pid_killer=CreateKiller(pid_primary);
		}
		else
		{
			//printf("\033[1;32m WATCHDOG: PRIMARY Still Alive\033[0m \n");
			fflush(stdout);
		}
	}
}
