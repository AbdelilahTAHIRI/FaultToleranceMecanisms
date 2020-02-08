#include <signal.h>
#include "watchdog.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

static int cout[2]={0};//one counter for each server

int IncCount( int i ) {
cout[i] += 1 ;
return ( cout[i] ) ;
}

int GetCount( int i ) {
return ( cout[i] ) ;
}

int ClearCount( int i ) {
cout[i] = 0 ;
return (cout[i]) ;
}

#define TIMEOUT 3

static void sigHandler(int sig)
{
	IncCount(0);
}

int main( int argc, char * argv[])
{
	int memory_counter;

	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
	{
		perror("signal");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		memory_counter=GetCount(0);
		sleep(TIMEOUT);
		if(memory_counter==GetCount(0))
		{
			printf("Server Process is dead\n");
			fflush(stdout);
			continue;
		}
		printf("Server Process is Alive\n");
		fflush(stdout);
		
	}
}
