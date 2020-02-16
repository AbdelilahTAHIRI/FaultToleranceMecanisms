/**
@Content: This file contains the high level process which launch the fault tolerant system. 
				  It creates the WATCHDOG, SENSOR processes and the required files to store Checkpoint.
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"
#include <errno.h>

int main( int argc, char * argv[])
{
	pid_t pid_sensor;
	pid_t pid_wtd;
	int memory_fd;
	int i;
	char str_pid_wtd[4];
	
	//Create a fifo for data exchange between SENSOR and SERVER
	mkfifo("./tmp/fifo1",S_IRUSR | S_IWUSR);
	//Create a file for Checkpointing
	memory_fd=open("./tmp/memory_stable",O_WRONLY | O_RDONLY | O_CREAT | O_SYNC, S_IRWXU);
	if(memory_fd == -1)
	{
		perror("MAIN: open memory_stable file");
		exit(EXIT_FAILURE);
	}
	//Create the WATCHDOG process
	pid_wtd=fork();
	switch(pid_wtd)
	{
		case -1: 
			perror("MAIN: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			printf("launch watchdog bin\n");;
			char *args[]={"watchdog",NULL};
			int ret;
			ret=execv("./bin/watchdog",args);
			if(ret==-1)
			{
				perror("MAIN: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}
	//Create the SENSOR process
	pid_sensor=fork();
	switch(pid_sensor)
	{
		case -1: 
			perror("MAIN: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			printf("launch sensor bin\n");
			char *args[]={"sensor",NULL};
			int ret;
			ret=execv("./bin/sensor",args);
			if(ret==-1)
			{
				perror("MAIN: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}

	while ( 1 )
	{
		pause();
	}
}
