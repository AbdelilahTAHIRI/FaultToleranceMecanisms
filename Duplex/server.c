/**
@Content: This file contains the server process implementation with it's two mode: PRIMARY and BACKUP
				  
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "server.h"
//The sliding window specific for each server
static float sliding_window[N]={0};
//The current mode of the server.
static int mode=PRIMARY;
//The output data retreived from the checkpoint.
static float output;
//This variable indicates either the readed output from the checkpoint was printed by the died PRIMARY or not
static int outputprinted;
//This variable control the Compute_Mean3() function.
static int wrong_mean2=0;
//This variable control the Compute_Mean3() function. 
static int wrong_mean3=0;
//this variable is set by the backup when it switchs to primary mode
static int switched=0; 
//Fifo descript
static int fifo_fd;
//This function initialize the sliding window with NaN values
void init_window(void)
{
	int i;
	for(i=0;i<N;i++)
	{
			sliding_window[i]=NaN;
	}
}
//This is the first version of the function computing the mean over the slinding window
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
//This function adds the passed sample to the sliding_window
void add_sample(float sample)
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
//This function print the result returned by the server
void print_result(float result)
{
	printf("\033[1;32m The Mean Value is %.2f \033[0m\n",result);
	fflush(stdout);
}
//This function prints the sliding_window of the server
void print_window(void)
{
	int i;
	for(i=0;i<10;i++)
	{
		printf("window[%d]=%.2f\t ",i,sliding_window[i]);
		fflush(stdout);
	}
	printf("\n");
}
//This function reads the outputprinted flag from the checkpoint
int ReadOutputIsPrinted(int fd)
{
	int ret;
	int buf;
	ret=read(fd, &buf, sizeof(buf));
	if(ret==-1)
	{
		if (errno == EINTR) // read is interruted by a signal handling
			return ReadOutputIsPrinted(fd);
		if (errno == EAGAIN) //there is no output value in checkpoint
			return 0;
		else
			return -1;
			perror("SERVER: read error");
	}
	else if(ret==0)
		return 0;
	else
		return buf;

}
//This function reads the output from the checkpoint
float ReadOutput(int fd)
{
	int ret;
	float buf;
	ret=read(fd, &buf, sizeof(buf));
	/*printf("ret=%d\n",ret);
	fflush(stdout);*/
	if(ret==-1)
	{
		if (errno == EINTR) // read is interruted by a signal handling
			return ReadOutput(fd);
		if (errno == EAGAIN) //there is no output value in checkpoint
			return NaN;
		else {
			return NaN;
			perror("SERVER: read error");}
	}
	else if(ret==0)
		return NaN;
	else
		return buf;

}
//This function reads the sliding window from the checkpoint
void ReadWindow(int fd)
{
	int ret;
	ret=read(fd,sliding_window,sizeof(sliding_window));

	if(ret==-1) 
	{
		if (errno == EINTR) // read is interruted by a signal handling
			ReadWindow(fd);
		if (errno == EAGAIN) //there is no input value in checkpoint
			{printf("\033[1;32m there is no input value in checkpoint \033[0m \n");
			fflush(stdout);}
			//cas grave, le serveur doit continuer le service avec les donnes internes qu'il a dans sa memoir interne
		else
			perror("SERVER: read error");
	}

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
//This function reads the checkpoint from the memory and store it in internal variables of the server		
void ReadCheckpoint(void)
{
	int memory_fd;
	int ret;
	memory_fd=open("./tmp/memory_stable",O_RDONLY | O_NONBLOCK);
	ReadWindow(memory_fd);
	output=ReadOutput(memory_fd);

	outputprinted=ReadOutputIsPrinted(memory_fd);

	close(memory_fd);	
}
//this function receives the data from the FIFO
int RevceiveSample(float * buff)
{
	int memory_fd;
	int ret;
	ret=read(fifo_fd,buff,sizeof(*buff));
	if(ret==-1)
	{
		if (errno == EINTR) // read is interruted by a signal handling
			return RevceiveSample(buff);
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

//This is the function executed by the BACKUP Server for fault recovering 
void recoveryhandler(void)
{
	ReadCheckpoint();
	/*print_window();
	printf("\noutput=%.2f\n",output);
	fflush(stdout);	*/
}

//Signals handler
static void sigHandler(int sig)
{
	if(sig==SIGUSR2)
	{
		//printf("\033[1;33m RECOVERY HANDLING \033[0m\n");
		mode=PRIMARY;
		switched=1;
		recoveryhandler();
		fflush(stdout);
	}
	else if(sig==SIGUSR1 && mode==PRIMARY)
	{
		if(wrong_mean2==0 && wrong_mean3==0)
			wrong_mean2=1;
		else if(wrong_mean2==1 && wrong_mean3==0)
			wrong_mean3=1;
	}
		
}

int main( int argc, char * argv[])
{
	float sample=0;
	float mean=0.0;
	int memory_fd;
	int ret;
	int printed;
	
	//Get the mode of the server
	mode=atoi(argv[1]);
	//initialize the sliding window with NaN values
	init_window();
	//open the fifo to get sensor from data
	fifo_fd=open("./tmp/fifo1", O_RDONLY);
	//install the SIGUSR1 handler
	if(signal(SIGUSR1 ,sigHandler)==SIG_ERR) 
	{
		perror("SERVER: signal");
		exit(EXIT_FAILURE);
	}
	
	//install the SIGUSR2 handler
	if(signal(SIGUSR2 ,sigHandler)==SIG_ERR) 
	{
		perror("SERVER: signal");
		exit(EXIT_FAILURE);
	}
	
	//Create Kick child process for PRIMARY mode
	if(mode==PRIMARY)
	{
		CreateKickProcess();
	}
	
	while (1)
	{
		//sleep(1);
		//PRIMARY Mode
		if(mode==PRIMARY) 
		{
			
			if(switched==0)
			{
				//receive new sample from the sensor
				ret=RevceiveSample(&sample);
				if(ret>0)
				{
					printf("PRIMARY: Deliver service\n");
					add_sample(sample);
					memory_fd=open("./tmp/memory_stable",O_WRONLY| O_TRUNC | O_SYNC);
					//delete the content of the stable memory
					ftruncate(memory_fd,0);
					//Store the sliding_window of the PRIMARY to the stable memory
					WriteWindow(memory_fd);
					//Service delivering
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
				//Create Kick child process for PRIMARY mode
				CreateKickProcess();
		
				if(isnan(output))//there is no output in the checkpoint readed.
				{
					//Compute the Mean based on the sliding window readed from the checkpoint
					mean=Compute_Mean();
					//Open the stable memory in append mode
					memory_fd=open("./tmp/memory_stable", O_WRONLY| O_TRUNC | O_SYNC);
					
					//Store the service output in the stable memory
					WriteOutput(memory_fd, mean);
					printf("PRIMARY: Deliver service\n");
					//print the service output
					print_result(mean);
					//Store the printing flag in the stable memory
					printed=1;
					WriteOutputIsPrinted(memory_fd,printed);
					close(memory_fd);
				}
				else if(outputprinted==0) //the readed checkpoint contains an output value that was not printed
				{
					printf("PRIMARY: Deliver service\n");
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
		else if (mode==BACKUP) //BACKUP Mode
		{
			sleep(1);
			printf("BACKUP: Save Checkpoint\n");
			//Continuously read of the stable memory and store it in the internal variables of the server
			ReadCheckpoint();
			fflush(stdout);
		}
	}
}


