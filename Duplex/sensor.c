#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "sensor.h"

int main( int argc, char * argv[])
{
	float buffer=0;
	int fd;
	int ret;
	fd=open("./tmp/fifo1", O_WRONLY);
	srand(time(NULL));   // Initialization, should only be called once.
	while (1)
	{
		sleep(1);
		buffer=5000.0;
		printf("\n\n\n\nSENSOR Process: \n\n\n\n");
		fflush(stdout);
		write:
		ret=write(fd,&buffer,sizeof(buffer));
		if(ret==-1)
			perror("SENSOR: write");
		else if(ret=!sizeof(buffer))
		{
			ftruncate (fd,  ret);
			goto write;
		}
	}
}
