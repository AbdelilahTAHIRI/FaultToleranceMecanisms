/**
@Content: This file contains the implementation of the process that will kill the PRIMARY Server.
				  This is a way to inject crash faults.
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include "killer.h"

int main( int argc, char * argv[])
{
	//Get the PID of the process to kill
	int pid_to_kill=atoi(argv[1]);
	//Wait some time
	sleep(13);
	//Kill the process with PID passed in argument
	kill(pid_to_kill, SIGKILL);
	//Notify the crash fault injection to the WATCHDOG ( for tie detection measurment)
	kill(getppid(), SIGUSR2);
	
	printf("\033[1;31mKILLER: PRIMARY Killed \033[0m\n");
	fflush(stdout);

	//Exit
	exit(EXIT_FAILURE);
}
