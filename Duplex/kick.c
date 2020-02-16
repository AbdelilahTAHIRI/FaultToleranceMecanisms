/**
@Content: This file contains the implementation of the watchdog kicking. This is the child process created by 
				  the PRIMARY server. This process kick the watchdog periodecally and will be died if its parent die.
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "kick.h"
//The kick function
void kick(int pid)
{
	//Send a SIGUSR1 signal to the WATCHDOG.
	kill(pid,SIGUSR1);
}

int main( int argc, char * argv[])
{
	//Get the WATCHDOG PID.
	int pid_wtd=atoi(argv[1]);
	int pid_primary;
	int pid;
	//Get the PRIMARY PID.
	pid_primary=getppid();
	
	while (1)
	{
		//Get the current parent PID
		pid=getppid();
		//If PRIMARY Server if not the current parent then exit
		if(pid!=pid_primary)
			exit(EXIT_FAILURE);
		else
		{
			//Wait a period of 1 second
			sleep(1);
			printf("KICK: kick the WATCHDOG\n");
			fflush(stdout);
			//Kick the WATCHDOG
			kick(pid_wtd);
		}
	}
}
