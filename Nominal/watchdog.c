#include <signal.h>
#include "watchdog.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>

static int cout=0;
static int ServerCount=1;
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
	IncCount();
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
	
	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
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
		sleep(2);//Attendre le temps que le primaire donne un coup de pied au watchdog
		if(memory_counter==GetCount())
		{
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
			printf("\033[1;32m WATCHDOG: PRIMARY Still Alive\033[0m \n");
			fflush(stdout);
		}
	}
}