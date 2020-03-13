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

//the internal watchdog counter
static int cout=0;
//timespec struct to measure detection time.
static struct timespec ts1;
static struct timespec ts2;
//Function that increment the internal counter 
int IncCount( ) {
cout += 1 ;
return ( cout ) ;
}
//Function to get internal counter 
int GetCount( void ) {
return ( cout ) ;
}
//This function creates a server process with the specified mode
int CreateServer(int mode)
{
	int pid;
	pid=fork();
	char str_mode[4];
	switch(pid)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_mode,"%d",mode);
			char *args[]={"server",str_mode,NULL};
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
//This function creates an injector process to introduce fault in the specified server process
int CreateInjector(int pid)
{
	int pid_injector;
	char str_pid[4];
	pid_injector=fork();
	switch(pid_injector)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_pid,"%d",pid);
			char *args[]={"injector",str_pid,NULL};
			int ret;
			ret=execv("./bin/injector",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}
	return pid_injector;
}
static inline void tsnorm(struct timespec *ts)
{
	while(ts->tv_nsec>= NSEC_PER_SEC)
	{
		ts->tv_nsec-=NSEC_PER_SEC;
		ts->tv_sec++;
	}
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
	pid_t pid_backup;
	pid_t pid_killer;
	pid_t pid_injector;
	char str_pid_primary[4];
	int ret;
	char mode[4];
	float delay;
	int i;
	int status;
	//install the SIGUSR1 handler
	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
	{
		perror("WATCHDOG: signal");
		exit(EXIT_FAILURE);
	}
	//install the SIGUSR2 handler
	if(signal(SIGUSR2 ,sigHandler)==SIG_ERR) 
	{
		perror("WATCHDOG: signal");
		exit(EXIT_FAILURE);
	}
	
	//Create PRIMARY
	pid_primary=CreateServer(PRIMARY);
	//Create BACKUP
	pid_backup=CreateServer(BACKUP);
	//Create Killer
	//pid_killer=CreateKiller(pid_primary);
	
	//Create Fault Injector Process
	pid_injector=CreateInjector(pid_primary);


	while (1)
	{
		//get the current counter
		memory_counter=GetCount();
		//sleep(TIMEOUT);//Attendre le temps que le primaire donne un coup de pied au watchdog
		for(i=0;i<150;i++)
		{usleep(1);}
		//If the counter is changed
		if(memory_counter==GetCount())
		{
			//Get the time detection
			ret = clock_gettime (CLOCK_MONOTONIC, &ts2);
			if (ret) {perror ("clock_gettime");}
			
			//Compute the Delay Time
			delay=(ts2.tv_sec-ts1.tv_sec)+(ts2.tv_nsec-ts1.tv_nsec)*0.0000000001;
			
			printf("\033[1;33m Detection time is %f sec \n",delay);
			printf("\033[1;33mWATCHDOG: PRIMARY is dead: ");

			//Send signal to BACKUP to Set it in PRIMARY mode
			kill(pid_backup, SIGUSR2);
			//Kill the PRIMARY in Failure
			kill(pid_primary, SIGKILL); 
			
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
			pid_primary=pid_backup; // changer le pid du primaire.
			pid_backup=CreateServer(BACKUP);//Creer un nouveau Backup 
			
			pid_injector=CreateInjector(pid_primary);
		}
		else
		{
			//printf("\033[1;32m WATCHDOG: PRIMARY Still Alive\033[0m \n");
			fflush(stdout);
		}
	}
}
