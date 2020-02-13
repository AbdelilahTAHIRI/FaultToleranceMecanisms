#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "include/server.h"
#include <math.h>
#include <signal.h>
#include "server.h"
#define N 10
//#define SIGUSR1 30

static float sliding_window[N]={0};

float ComputeMean(void)
{
	float mean=0.0;
	int count=0;
	int i;
	for(i=0;i<N;i++)
	{
		if(!isnan(sliding_window[i]))
		{
			mean+=sliding_window[i];
			count++;
		}
	}
	if(count!=0)
		mean/=count;
	else
		mean=0;
	return mean;
}
void add_sample(int sample)
{
	int i;
	for(i=0;i<N;i++)
	{
		if(isnan(sliding_window[i]))
		{
			sliding_window[i]=sample;
			return;
		}
	}
	for(i=0;i<N-1;i++)
	{
		sliding_window[i]=sliding_window[i+1];
	}
	sliding_window[N-1]=sample;
	return;	
}

void print_result(float result)
{
	printf("The Mean Value is %.2f",result);
}
int StoreWindow(void)
{
	int memory_fd;
	memory_fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/memory_stable",O_WRONLY);	
	write(memory_fd,sliding_window,sizeof(sliding_window));
	close(memory_fd);
}

int StoreOutput(float output)
{
	int memory_fd;
	memory_fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/memory_stable",O_APPEND);	
	write(memory_fd,output,sizeof(output));
	close(memory_fd);
}


int main( int argc, char * argv[])
{
	int sample=0;
	int fd;
	float mean=0.0;
	int pid_wtd;
	pid_wtd=atoi(argv[1]);
	int memory_fd;


	fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/fifo1", O_RDONLY);
	while (1)
	{
		sleep(1);
		
		printf("\n\n\n\nI am the server\n");
		if(read(fd,&sample,sizeof(sample))>0)
		{
			printf("I read %d from the FIFO\n\n\n\n",sample);
			fflush(stdout);
			kill(pid_wtd,SIGUSR1);
			add_sample(sample);
			memory_fd=open("/home/m2istr_21/FaultToleranceMecanisms/Nominal/tmp/memory_stable",O_WRONLY);	
			write(memory_fd,output,sizeof(output));
			mean=ComputeMean();
			write(memory_fd,mean,sizeof(mean));
			close(memory_fd);
			print_result(mean);
		}

	}
}


