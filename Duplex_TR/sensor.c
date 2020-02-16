/**
@Content: This file contains the SENSOR process implementation. 
				  It creates the sends floats via the fifo to the PRIMARY Server
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "sensor.h"
//Vector containing the data tha will be transmitted to the PRIMARY Server
static float samples[10];

int main( int argc, char * argv[])
{
	float buffer=0;
	int fifo;
	int ret;
	int i;
	for(i=0;i<10;i++)
	{
		samples[i]=i+1;
	}
	//Open the fifo for write
	fifo=open("./tmp/fifo1", O_WRONLY);
	//srand(time(NULL));   // Initialization, should only be called once.
	i=0;
	while (1)
	{
		sleep(1);
		//browse the samples[] vector circularly
		if(i<=9)
			buffer=samples[i];
		else {
			i=0;
			buffer=samples[i];
		}
		i++;
		
		printf("\nSENSOR: Deliver a new value\n");
		fflush(stdout);
		//send the sample in the FIFO
		ret=write(fifo,&buffer,sizeof(buffer));
		if(ret==-1)
			perror("SENSOR: write");
	}
}
