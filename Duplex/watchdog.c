#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "watchdog.h"

static int cout=0;//one counter for each server
static int filedes[2]; 
int IncCount( ) {
cout += 1 ;
return ( cout ) ;
}

int GetCount( void ) {
return ( cout ) ;
}

int ClearCount( int i ) {
cout = 0 ;
return (cout) ;
}

static void sigHandler(int sig)
{
	IncCount();
}

int main( int argc, char * argv[])
{
	int memory_counter;
	pid_t pid_primary;
	pid_t pid_backup;
	pid_t pid_killer;
	char str_pid_primary[4];
	char mode[4];
	
	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
	{
		perror("WATCHDOG: signal");
		exit(EXIT_FAILURE);
	}
	
	pid_primary=fork();
	switch(pid_primary)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(mode,"%d",PRIMARY);
			char *args[]={"server",mode,NULL};
			int ret;
			ret=execv("./bin/server",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the watchdog\n\n\n\n");
			break;			
	}
	pid_backup=fork();
	switch(pid_backup)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(mode,"%d",BACKUP);
			char *args[]={"server",mode,NULL};
			int ret;
			ret=execv("./bin/server",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the watchdog\n\n\n\n");
			break;			
	}
	
	pid_killer=fork();
	switch(pid_killer)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_pid_primary,"%d",pid_primary);
			char *args[]={"killer",str_pid_primary,NULL};
			int ret;
			ret=execv("./bin/killer",args);
			if(ret==-1)
			{
				perror("WATCHDOG: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the watchdog\n\n\n\n");
			break;			
	}


	while (1)
	{
		memory_counter=GetCount();
		sleep(TIMEOUT);//Attendre le temps que le primaire donne un coup de pied au watchdog
		if(memory_counter==GetCount())
		{
			printf("WATCHDOG: Server Process is dead\n");
			fflush(stdout);
			kill(pid_backup, SIGUSR2); //Envoyer un signal au backup pour le mettre en mode primaire
			kill(pid_primary, SIGKILL); //tuer le primaire defaillant
			pid_primary=pid_backup; // changer le pid du primaire.
			pid_backup=fork();//Creer un nouveau processus serveur en mode Backup
			switch(pid_backup)
			{
				case -1: 
					perror("WATCHDOG: fork");
					exit(EXIT_FAILURE);
				case 0 : 
					sprintf(mode,"%d",BACKUP);
					char *args[]={"server",mode,NULL};
					int ret;
					ret=execv("./bin/server",args);
					if(ret==-1)
					{
						perror("WATCHDOG: execv");
						exit(EXIT_FAILURE);
					}
					break;
				default:
					printf("\n\n\n\nI am the watchdog\n\n\n\n");
					break;			
			}
		}
		else
		{
			printf("WATCHDOG: Server Process is Alive\n");
			fflush(stdout);
		}		
	}
}
