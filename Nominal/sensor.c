#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "sensor.h"

int main( int argc, char * argv[])
{
	int buffer=0;
	int fd;
	fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/fifo1", O_WRONLY);
	srand(time(NULL));   // Initialization, should only be called once.
	while (1)
	{
		sleep(1);
		//buffer = rand();
		buffer=5000;
		printf("\n\n\n\nI am the sensor\n\n\n\n");
		
		write(fd,&buffer,sizeof(buffer));
		printf("I write %d in the FIFO\n\n\n\n",buffer);
		fflush(stdout);
	}
}
