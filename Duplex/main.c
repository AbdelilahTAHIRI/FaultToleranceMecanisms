#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "main.h"

int mkfifo(const char *pathname, mode_t mode);

int main( int argc, char * argv[])
{
	float tab[10]={0};
	pid_t pid_sensor;
	pid_t pid_server;
	pid_t pid_wtd;
	int memory_fd;
	int i;
	char str_pid_wtd[4];
	float output;
	mkfifo("./tmp/fifo1",S_IRUSR | S_IWUSR);
	memory_fd=open("./tmp/memory_stable",O_WRONLY | O_RDONLY | O_CREAT | O_SYNC, S_IRWXU);
	if(memory_fd == -1)
	{
		perror("MAIN: open memory_stable file");
		exit(EXIT_FAILURE);
	}

	pid_wtd=fork();
	switch(pid_wtd)
	{
		case -1: 
			perror("MAIN: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			;
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
			printf("\n\n\n\nMAIN process\n\n\n\n");
			break;			
	}
	
	pid_sensor=fork();
	switch(pid_sensor)
	{
		case -1: 
			perror("MAIN: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			;
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
			printf("\n\n\n\nMAIN process\n\n\n\n");
			break;			
	}

	while ( 1 )
	{
		sleep(1);
		printf("\n\n\n\nMAIN process\n\n\n\n");
		memory_fd=open("./tmp/memory_stable",O_RDONLY | O_NONBLOCK);	
		read(memory_fd,tab,sizeof(tab));
		if(read(memory_fd, &output, sizeof(output))==0) //there is no output in chackpoint.
			output=NaN;
		close(memory_fd);
		printf("\n");
		for(i=0;i<10;i++)
		{
			printf("tab[%d]=%f\t ",i,tab[i]);
			fflush(stdout);
		}
		printf("\n");
		printf("\n");
	}
}
