# define _XOPEN_SOURCE 600

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

int main ( void )
{
	struct addrinfo *results, hints = {0};

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int errcode = getaddrinfo("127.0.0.1", "6667", &hints, &results);
	if ( errcode )
	{
		fprintf(stderr, "Could not ping 127.0.0.1:6667.");
		return 3;
	}
	int sd = socket(results->ai_family, results->ai_socktype, 0);
	if(sd < 0) 
	{
		perror("Could not open socket");
		freeaddrinfo(results);
		return 4;
	}

	if(connect(sd, results->ai_addr, results->ai_addrlen) < 0) 
	{
		perror("Could not connect to remote.");
		close(sd);
		freeaddrinfo(results);
		return 4;
	}
	char input[201] = {"HELP"};
	char buffer[200] = {"\0"};
	char quitval[] = "QUIT";
	ssize_t read_status;
	
	struct passwd *p = getpwuid(getuid());
	char * name = p->pw_name;
	char USERCommand[200] = {'\0'};
	strcat(USERCommand, "USER ");
	strcat(USERCommand, name);
	strcat(USERCommand, " 0 127.0.0.1 :");
	strcat(USERCommand, name);
	strcat(USERCommand, "\r\n");
	printf("%s\n", USERCommand);
	char NICK[100] = {'\0'};
	strcpy(NICK, "NICK ");
	strcat(NICK, name);
	strcat(NICK, "\n");

	sleep(1);
	if ( write ( sd, USERCommand, strlen(USERCommand)) < 0 )
	{
			perror("Could not grab user name.");
			close(sd);
			freeaddrinfo(results);
			return 6;
	}
	if ( write ( sd, NICK, strlen(NICK)) < 0 )
	{
			perror("Could not write nickname.");
			close(sd);
			freeaddrinfo(results);
			return 7;
	}
	
	while((read_status = read(sd, buffer, sizeof(buffer)-1)) == 199)
	{
		buffer[read_status] = '\0';
		printf("%s", buffer);
	}

	freeaddrinfo(results);

	pid_t pid = fork();
	
	if (pid == 0)
	{
		while ( strcmp(input, quitval ))
		{
			fgets(input, 200, stdin);
			if(write(sd, input, strlen(input)) < 0) 
			{
				perror("Could not write to remote");
				close(sd);
				freeaddrinfo(results);
				return 5;
			}
		}	
		close(sd);
	}
	else if (pid > 0)
	{
		while (sd)
		{
			while((read_status = read(sd, buffer, sizeof(buffer)-1)) == 199) 
			{		
				buffer[read_status] = '\0';	
				printf("%s", buffer);
				memset(buffer, '\0', strlen(buffer));
			}
			if ( read_status > 0 )
			{
				if (buffer[0] == 'P' && buffer[1] == 'I' && \
					buffer[2] == 'N' && buffer[3] == 'G')
				{
					char pong[100] = {'\0'};
					strcat(pong, "PONG ");
					strcat(pong, &buffer[5]);

					strcat(pong, "\n");
					printf("Ponging.\n");
					if ( write(sd,pong,strlen(pong)) < 0)
					{
						perror("Could not write to remote");
						close(sd);
						freeaddrinfo(results);
						return 500;
					}
					memset(buffer, '\0', strlen(buffer));
				}

				printf("%s", buffer);
				printf("\n");
			}
			memset(buffer, '\0', strlen(buffer));
		}
	}
	return 0;
}
