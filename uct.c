# define _XOPEN_SOURCE 600

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <pwd.h>

#include <stdlib.h>
#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

int main ( void )
{
	//Set up socket structure that will contain target port info
	struct addrinfo *results, hints = {0};
	//Specify hints structures, used to get information for results field?
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Fill in RESULTS structure.
	int errcode = getaddrinfo("127.0.0.1", "6667", &hints, &results);
	if ( errcode )
	{
		//If getaddrinfo failed --
		fprintf(stderr, "Could not ping 127.0.0.1:6667.");
		return 3;
	}
	//Actually bind socket.
	int sd = socket(results->ai_family, results->ai_socktype, 0);
	if(sd < 0) 
	{
		//If socket could not be bound, fail out.
		perror("Could not open socket");
		freeaddrinfo(results);
		return 4;
	}

	//Attempt to connect to thing held in results
	if(connect(sd, results->ai_addr, results->ai_addrlen) < 0) 
	{
		//IF no connection could be made, fail out.
		perror("Could not connect to remote.");
		close(sd);
		freeaddrinfo(results);
		return 4;
	}

	//Declarations for actual program.
	//Default input command -- Not sent; doesn't matter.
	//Currently a silent cry for help that none shall see.
	char input[201] = {"HELP"};
	//Set default for BUFFER.  0'd out.
	char buffer[200] = {"\0"};
	//Declare variable READ_STATUS
	ssize_t read_status;
	
	//Get user ID.
	struct passwd *p = getpwuid(getuid());
	char * name = p->pw_name;

	//Assign USER command to be USER <userid> 0 127.0.0.1 <userid>
	char USERCommand[200] = {'\0'};
	strcat(USERCommand, "USER ");
	strcat(USERCommand, name);
	strcat(USERCommand, " 0 127.0.0.1 :");
	strcat(USERCommand, name);
	strcat(USERCommand, "\r\n");
	printf("%s\n", USERCommand);

	//Assign NICK to be NICK <userid>
	char NICK[15] = {'\0'};
	strncpy(NICK, "NICK ", 6);
	strncat(NICK, name, 9);
	NICK[14] = '\n';
	NICK[15] = '\0';

	//Sleep for 1 second to allow server time to respond.
	sleep(1);

	//Send USER command to server.
	if ( write ( sd, USERCommand, strlen(USERCommand)) < 0 )
	{
			//If error, close out.
			perror("Could not grab user name.");
			close(sd);
			freeaddrinfo(results);
			return 6;
	}
	if ( write ( sd, NICK, strlen(NICK)) < 0 )
	{
			//If error, close out.
			perror("Could not write nickname.");
			close(sd);
			freeaddrinfo(results);
			return 7;
	}
	//Sleep 2 seconds, for reasons.  Not sure why I have this here.
	//I'm afraid to remove it.  It definately allowed the output to
	//be nice and timely.
	sleep(2);

	//Read in hte "Logged on" part.
	while((read_status = read(sd, buffer, sizeof(buffer)-1)) == 199)
	{
		buffer[read_status] = '\0';
		printf("%s", buffer);
	}

	//Free results (so we don't have multiple copies floating around.
	freeaddrinfo(results);

	//Fork program into sender and receiver
	pid_t pid = fork();
	

	if (pid == 0)
	{
		//While they don't put in some variation of "QUIT"
		while (strcmp(input, "Quit\n") != 0 && strcmp(input,"QUIT\n") != 0 \
				&& strcmp(input, "quit\n") != 0)
		{
			//Read in their input, and send ti to the machine.
			//If unable to write to remote, close
			fgets(input, 200, stdin);
			if(write(sd, input, strlen(input)) < 0) 
			{
				perror("Could not write to remote");
				close(sd);
				kill(pid, SIGHUP);
				return 5;
			}
		}

		close(sd);
		kill(pid, SIGHUP);
	}
	else if (pid > 0)
	{
		//For forever, sadly  turns out closing the socket descriptor
		//sets it to 3, not 0.  I'm a bit upset about that but I
		//digress.
		while (sd)
		{
			//Try to read in from remote, if the size of buffer, go again
			while((read_status = read(sd, buffer, sizeof(buffer)-1)) == 199) 
			{		
				//Set the last byte to nullm print it, set buffer to null
				buffer[read_status] = '\0';	
				printf("%s", buffer);
				memset(buffer, '\0', strlen(buffer));
			}
			//If there's still data in read_status (Or less than 199
			//was read in)
			if ( read_status > 0 )
			{
				//check for servers PING
				if (buffer[0] == 'P' && buffer[1] == 'I' && \
					buffer[2] == 'N' && buffer[3] == 'G')
				{
					//Make string PONG (Whoever sent ping)
					char pong[100] = {'\0'};
					strcat(pong, "PONG ");
					strcat(pong, &buffer[5]);
					strcat(pong, "\n");

					//printf("Ponging.\n");

					//Send server PONG request to stay alive.
					if ( write(sd,pong,strlen(pong)) < 0)
					{
						//If error, print error, close.
						perror("Could not write to remote");
						close(sd);
						freeaddrinfo(results);
						return 500;
					}
					//Reset BUFFER to 0s, purely in PONG. Else no touchy
					memset(buffer, '\0', strlen(buffer));
				}
				//Print BUFFER
				printf("%s", buffer);
				printf("\n");
			}
			//Reset Buffer to 0s.
			memset(buffer, '\0', strlen(buffer));
		}
	}
	return 0;
}
