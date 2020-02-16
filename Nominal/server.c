#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "server.h"

#include <time.h>

static float sliding_window[N]={0};
static float output;
static int outputprinted;
static int switched=0; 
static int fifo_fd;
void init_window(void)
{
	int i;
	for(i=0;i<N;i++)
	{
			sliding_window[i]=NaN;
	}
}

float Compute_Mean(void)
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
	printf("\033[1;32mThe Mean Value is %.2f \033[0m",result);
	fflush(stdout);
}

void ReadCheckPoint(void)
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
			//cas grave, le serveur commence de collecter les donnees a nouveau
		else
			perror("SERVER: read error");
	}
	else if(ret==0)
	{
		printf("\nthere is no input value in checkpoint\n");
		//cas grave, le serveur commence de collecter les donnees a nouveau
		init_window();
		output=NaN;
		return ;
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
	else if(ret==0)  //there is no output value in checkpoint
	{
		output=NaN;
	}
	
	read3:
	ret=read(memory_fd, &outputprinted, sizeof(outputprinted));
	if(ret==-1) 
	{
		if (errno == EINTR) // read is interruted by a signal handling
		goto read3;
		if (errno == EAGAIN) //there is no output value in checkpoint
			outputprinted=0;
		else
		{
			perror("SERVER: read error");
			outputprinted=0;
		}
	}
	else if(ret==0)  //there is no output value in checkpoint
	{
		outputprinted=0;
	}
	
	close(memory_fd);	
}

//This function writes the sliding window to the checkpoint
void WriteWindow(int fd)
{
	int ret;
	ret=write(fd,sliding_window,sizeof(sliding_window));

	if(ret==-1) 
	{
		if (errno == EINTR) // write is interruted by a signal handling
			WriteWindow(fd);
		else
			perror("SERVER: write1");
	}
}

//This function writes the outputprinted flag to the checkpoint
void WriteOutputIsPrinted(int fd,int out)
{
	int ret;
	ret=write(fd,&out,sizeof(out));
	if(ret==-1) 
	{
		if (errno == EINTR) // write is interruted by a signal handling
			WriteOutputIsPrinted(fd,out);
		else
			perror("SERVER: write2");
	}
}		
//This function writes the output to the checkpoint
void WriteOutput(int fd,float out)
{
	int ret;
	ret=write(fd,&out,sizeof(out));
	if(ret==-1) 
	{
		if (errno == EINTR) // write is interruted by a signal handling
			WriteOutput(fd,out);
		else
			perror("SERVER: write3");
	}
}
//this function receives the data from the FIFO
int ReceiveSample(float * buff)
{
	int memory_fd;
	int ret;
	ret=read(fifo_fd,buff,sizeof(*buff));
	if(ret==-1)
	{
		if (errno == EINTR) // read is interruted by a signal handling
			return ReceiveSample(buff);
		if (errno == EAGAIN) //there is no sample in FIFO
			return ret;
		else {
			perror("SERVER: read error");
			return ret;	
			}	
	}
	return ret;	
}

//This function Creates a child process that perform the WATCHDOG kicking
void CreateKickProcess(void)
{
	char str_pid_wtd[4];
	int pid_child;
	int pid_wtd;
	pid_wtd=getppid();
	pid_child=fork();
	switch(pid_child)
	{
		case -1: 
			perror("WATCHDOG: fork");
			exit(EXIT_FAILURE);
		case 0 : 
			sprintf(str_pid_wtd,"%d",pid_wtd);
			char *args[]={"kick",str_pid_wtd,NULL};
			int ret;
			ret=execv("./bin/kick",args);
			if(ret==-1)
			{
				perror("SERVER: execv");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			break;			
	}
	return;
}


int main( int argc, char * argv[])
{
	float sample=0;
	float mean=0.0;
	int pid_wtd;
	int memory_fd;
	int ret;
	int printed;
	int server_index=atoi(argv[1]);
	
	fflush(stdout);
	if(server_index==1) //the first created server
	{
		//initialize the sliding window with NaN values
		init_window();
		output=NaN;
		switched=0;
	}
	else 
	{
		printf("ReadCheckPoint\n");
		fflush(stdout);
		ReadCheckPoint();
		switched=1;
	}
	CreateKickProcess();
	fflush(stdout);
	fifo_fd=open("./tmp/fifo1", O_RDONLY);
	
	while (1)
	{
		//sleep(1);
		printf("\nPRIMARY: Deliver service\n");
			if(switched==0)
			{
				//receive new sample from the sensor
				ret=ReceiveSample(&sample);
				if(ret>0)
				{
					add_sample(sample);
					memory_fd=open("./tmp/memory_stable",O_WRONLY| O_TRUNC | O_SYNC);
					//delete the content of the stable memory
					ftruncate(memory_fd,0);
					//Store the sliding_window of the PRIMARY to the stable memory
					WriteWindow(memory_fd);
					//Service delevering
					mean=Compute_Mean();
					//Store the service output in the stable memory
					WriteOutput(memory_fd, mean);
					//print the service output
					print_result(mean);
					//Store the printing flag in the stable memory
					printed=1;
					WriteOutputIsPrinted(memory_fd,printed);
					close(memory_fd);
				}
			}
			else if(switched==1)//The server just changed the mode from PRIMARY to BACKUP
			{
				switched=0;//Reinit the switched variable.
				
				if(isnan(output))//there is no output in the checkpoint readed.
				{
					//Compute the Mean based on the sliding window readed from the checkpoint
					mean=Compute_Mean();
					//Open the stable memory in append mode
					memory_fd=open("./tmp/memory_stable", O_WRONLY| O_TRUNC | O_SYNC);
					
					//Store the service output in the stable memory
					WriteOutput(memory_fd, mean);
					//print the service output
					print_result(mean);
					//Store the printing flag in the stable memory
					printed=1;
					WriteOutputIsPrinted(memory_fd,printed);
					close(memory_fd);
				}
				else if(outputprinted==0) //the readed checkpoint contains an output value that was not printed
				{
					print_result(output);
					continue;
				}
				else if(outputprinted==1)//the readed checkpoint contains an output value that was printed
				{
					continue;
				}

			}
			fflush(stdout);
	}
}


