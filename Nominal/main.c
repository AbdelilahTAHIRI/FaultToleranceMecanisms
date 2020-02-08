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
	mkfifo("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/fifo1",S_IRUSR | S_IWUSR);
	memory_fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/memory_stable",O_WRONLY | O_RDONLY | O_CREAT | O_SYNC, S_IRWXU);
	if(memory_fd == -1)
	{
		perror("open memory_stable file");
		exit(EXIT_FAILURE);
	}

	pid_wtd=fork();
	switch(pid_wtd)
	{
		case -1: 
			perror("fork");
			exit(EXIT_FAILURE);
		case 0 : 
			;
			char *args[]={"watchdog",NULL};
			int ret;
			ret=execv("/home/m2istr_21/FaultToleranceMecanisms/Nominal/bin/watchdog",args);
			if(ret==-1)
			{
				perror("execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the parent\n\n\n\n");
			break;			
	}
	
	pid_sensor=fork();
	switch(pid_sensor)
	{
		case -1: 
			perror("fork");
			exit(EXIT_FAILURE);
		case 0 : 
			;
			char *args[]={"sensor",NULL};
			int ret;
			ret=execv("/home/m2istr_21/FaultToleranceMecanisms/Nominal/bin/sensor",args);
			if(ret==-1)
			{
				perror("execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the parent\n\n\n\n");
			break;			
	}

	sprintf(str_pid_wtd, "%d", pid_wtd);
	pid_server=fork();
	switch(pid_server)
	{
		case -1: 
			perror("fork");
			exit(EXIT_FAILURE);
		case 0 : 
			;
			char *args[]={"server",str_pid_wtd,NULL};
			int ret;
			ret=execv("/home/m2istr_21/FaultToleranceMecanisms/Nominal/bin/server",args);
			if(ret==-1)
			{
				perror("execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printf("\n\n\n\nI am the parent\n\n\n\n");
			break;			
	}
	while ( 1 )
	{
		sleep(1);
		printf("\n\n\n\nin while loop :I am the parent\n\n\n\n");
		memory_fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/memory_stable",O_RDONLY);	
		read(memory_fd,tab,sizeof(tab));
		close(memory_fd);
		printf("\n");
		for(i=0;i<10;i++)
		{
			printf("tab[%d]=%f\t ",i,tab[i]);
			fflush(stdout);
		}
		printf("\n");
		printf("\n");

		printf("SENT PID_WATCHDOG %d\n",pid_wtd);
		fflush(stdout);
	}
}
