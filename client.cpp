// Brandon Wong
// CPSC3500
// client.cpp
// 6/3/2021


#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <limits>

using namespace std;

void error(const char* msg)
{
	perror(msg);
	exit(1);
}

struct Leaderboard
{
	string clientname = "temp";
	int turnCounter = numeric_limits<int>::max();
};

//function prototypes
bool checkInput(char*);

int main(int argc, char* argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent* server;

	char buffer[256];
	if (argc < 3)
	{
		printf("Too few command line arguments!\nPlease enter the IP address of the server and port of the server process.\n");
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error("ERROR opening socket");
	}

	server = gethostbyname(argv[1]);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR connecting");
	}

	//receiving leaderboard size from server
	bzero(buffer, 256);
	n = read(sockfd, buffer, 255);
	if (n < 0)
	{
		error("ERROR reading from socket");
	}
	string lb = "";
	for (long unsigned int i = 0; i < strlen(buffer); i++)
	{
		if (buffer[i] != '\n') lb += buffer[i];
	}
	int leaderboardsize = stoi(lb);
	bzero(buffer, 256);

	printf("\nWelcome to Number Guessing Game! \n");
	printf("Enter your name: ");
	bzero(buffer, 256);
	fgets(buffer, 255, stdin);
	//send client name to the server
	n = write(sockfd, buffer, strlen(buffer));
	if (n < 0)
	{
		error("ERROR writing to socket");
	}

	bool correctGuess = false;
	int turnCounter = 0;

	while (!correctGuess)
	{
		turnCounter++;
		printf("\n\nTurn: %d \n", turnCounter);
		
		printf("Enter a guess: ");
		bzero(buffer, 256);
		fgets(buffer, 255, stdin);
		char userInput[256];
		bzero(userInput, 256);
		for (long unsigned int i = 0; i < strlen(buffer); i++)
		{
			if (buffer[i] != '\n')
			{
				userInput[i] = buffer[i];
			}
		}
		//validate the guess
		while (!checkInput(userInput))
		{
			printf("Guess must be an integer within 0-999\n");
			printf("Enter a new guess: ");
			bzero(buffer, 256);
			fgets(buffer, 255, stdin);
			bzero(userInput, 256);
			for (long unsigned int i = 0; i < strlen(buffer); i++)
			{
				if (buffer[i] != '\n')
				{
					userInput[i] = buffer[i];
				}
			}
		}
		//send the guess to the server
		long hostInt = stol(userInput);
		long networkInt = htonl(hostInt);
		int bytesSent = send(sockfd, (void*)&networkInt, sizeof(long), 0);
		if (bytesSent != sizeof(long))
		{
			error("ERROR int not sent from client to server");
		}
		//receive result of guess from server
		bzero(buffer, 256);
		n = read(sockfd, buffer, 255);
		if (n < 0)
		{
			error("ERROR reading from socket");
		}
		//check if the guess was the Correct guess
		if (buffer[0] == 'C')
		{
			correctGuess = true;
		}
		printf("Result of the guess: %s\n", buffer);
	}
	//read victory message from server indicating number of turns it took
	bzero(buffer, 256);
	n = read(sockfd, buffer, 255);
	if (n < 0)
	{
		error("ERROR reading from socket");
	}
	printf("\n\n%s\n\n", buffer);
	
	//read and the print leaderboard from the server
	printf("Leaderboard:");
	for (int i = 0; i < leaderboardsize; i++)
	{
		bzero(buffer, 256);
		int n = read(sockfd, buffer, 255);
		if (n < 0)
		{
			error("ERROR reading from socket");
		}
		printf("%s", buffer);
	}
	printf("\n");
	

	//close the connection with the server
	close(sockfd);
	return 0;
}

bool checkInput(char* input)
{
	//first check if the input is an integer
	for (long unsigned int i = 0; i < strlen(input); i++)
	{
		if (!isdigit(input[i]) || isspace(input[i]))
		{	
			return false;
		}
	}
	//if input is an integer, check if the input is within 0-999
	int inputNum = stoi(input);
	if (inputNum < 0 || inputNum > 999)
	{
		return false;
	}
	return true;
}

/*
int readUInt32(int sock, uint32_t& val)
{
	int x = readAll(sock, &val, sizeof(val));
	if (x <= 0) return x;
	int value = ntohl(val);
	return value;
}
*/