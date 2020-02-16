/**
@Content: This file contains the implementation of the Fault injector process. 
				It Sends Signals to the PRIMARY Server to make the
**/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "injector.h"

int main( int argc, char * argv[])
{
	//Get the primary PID.
	int pid_primary=atoi(argv[1]);
	//Wait 12 seconds
	sleep(12);
	//Send signal to make the Compute_Mean2() function faulty. 
	kill(pid_primary,SIGUSR1);
	//Wait 2 seconds
	sleep(2);
	//Send signal to make the Compute_Mean2() function faulty. 
	kill(pid_primary,SIGUSR1);
	//Exit
	exit(EXIT_SUCCESS);
	
}