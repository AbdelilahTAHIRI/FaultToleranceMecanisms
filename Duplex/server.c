#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "include/server.h"
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "server.h"
//#include "watchdog.h"

static float sliding_window[N]={0};
static int mode=PRIMARY;
static float output;
void init_window(void)
{
	int i;
	for(i=0;i<N;i++)
	{
			sliding_window[i]=NaN;
	}
}

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
	fflush(stdout);
}
void recoveryhandler(void)
{
	int memory_fd;
	int ret;
	memory_fd=open("./tmp/memory_stable",O_RDONLY | O_NONBLOCK);
	read1:
	ret=read(memory_fd,sliding_window,sizeof(sliding_window));
	if(ret==-1) 
	{
		if (errno == EINTR) // read is interruted by a signal handling
		goto read1; 
		if (errno == EAGAIN) //there is no input value in checkpoint
			printf("\nthere is no input value in checkpoint\n");
			//cas grave, le serveur doit continuer le service avec les donnes internes qu'il a dans sa memoir interne
		else
			perror("SERVER: read error");
	}
	
	read2:
	ret=read(memory_fd, &output, sizeof(output));
	if(ret==-1) 
	{
		if (errno == EINTR) // read is interruted by a signal handling
		goto read2;
		if (errno == EAGAIN) //there is no output value in checkpoint
			output=NaN;
		else
			perror("SERVER: read error");
	}
	close(memory_fd);	
}

static void sigHandler(int sig)
{
	if(mode==PRIMARY)
		mode=BACKUP;
	else if(mode==BACKUP)
		mode=PRIMARY;
	recoveryhandler();
}

int main( int argc, char * argv[])
{
	float sample=0;
	int fd;
	float mean=0.0;
	int pid_wtd;
	int memory_fd;
	int ret;
	
	//initialize the sliding window with NaN values
	init_window();
	output=NaN;
	//pid_wtd=atoi(argv[1]);//Get the PID of the warchdog process
	mode=atoi(argv[1]);//Get the mode of the server
	
	//If the server mode is primary so open the fifo to get sensor from data
	if(mode==PRIMARY)
		fd=open("./tmp/fifo1", O_RDONLY);
	
	//install the SIGUSR1 handler
	if(signal(SIGUSR2 ,sigHandler)==SIG_ERR) 
	{
		perror("SERVER: signal");
		exit(EXIT_FAILURE);
	}
	while (1)
	{
		sleep(1);
		if(mode==PRIMARY)
		{
			printf("\n\n\n\n Primary Mode\n");
			if(isnan(output))
			{
				read_sensor:
				if(read(fd,&sample,sizeof(sample))>0)
				{
					kill(getppid(),SIGUSR1);
					add_sample(sample);
					memory_fd=open("./tmp/memory_stable",O_WRONLY| O_TRUNC | O_SYNC);
					rewrite_window:
					ret=write(memory_fd,sliding_window,sizeof(sliding_window));
					if(ret==-1)
						perror("SERVER: write");
					else if(ret=!sizeof(sliding_window)) //the sliding window is not written to the stable memory.
					{
						ftruncate (memory_fd,  ret);//delete the written item
						goto rewrite_window;
					}
					
					mean=ComputeMean();
					
					rewrite_output:
					ret=write(memory_fd,&mean,sizeof(mean));
					if(ret==-1)
						perror("SERVER: write");
					else if(ret=!sizeof(mean)) //the mean is not written to the stable memory.
					{
						ftruncate (memory_fd,  ret);//delete the written item
						goto rewrite_output;
					}
					
					close(memory_fd);
					
					print_result(mean);
				}
			}
			else //the primary has already computed the mean value so we will just print it
			{
				print_result(mean);
				goto read_sensor;
			}
		}
		else if (mode==BACKUP)
		{
			printf("\nBACKUP Mode\n");
			fflush(stdout);
			//Continuously read of the stable memory and store it in the internal variables of the server
			recoveryhandler();

		}
	}
}


