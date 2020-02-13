#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include "killer.h"

int main( int argc, char * argv[])
{
	int pid_to_kill=atoi(argv[1]);
	
	sleep(20);
	//Tuer le processus en argument
	kill(pid_to_kill, SIGKILL);
	printf("PRIMARY Server Killed\n");
	fflush(stdout);
	//Se cuicider
	raise(SIGKILL);
}
