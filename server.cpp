// Brandon Wong
// CPSC3500
// server.cpp
// 6/3/2021


#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>
#include <pthread.h>
#include<string>
#include <semaphore.h>
#include <limits>

using namespace std;

void error(const char* msg)
{
	perror(msg);
	exit(1);
}

struct ThreadArgs
{
	int clientSock;
	string clientName;
	int turnCounter;
};

struct Leaderboard
{
	string clientname = "temp";
	int turnCounter = numeric_limits<int>::max();
};

//global variables
const int LEADERBOARD_SIZE = 5;
Leaderboard lb[LEADERBOARD_SIZE];

//function prototypes
void* threadFunction(void*);
void updateLeaderboard(string, int);
void sendLeaderboard(int);
//bool sendUInt32(int, uint32_t);

//semaphore variables
int client_count = 0;
sem_t send_mutex;
sem_t update_mutex;

int main(int argc, char* argv[])
{
	//initializing variables
	sem_init(&send_mutex, 0, 1);
	sem_init(&update_mutex, 0, 1);
	int sockfd, newsockfd, portno;
	struct sockaddr_in cli_addr;
	struct sockaddr_in serv_addr; 
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error("ERROR opening socket");
	}
	bzero((char*)&serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}
	listen(sockfd, 5);

	while (true)
	{
		//create a thread to handle a new client
		socklen_t clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*) & cli_addr, &clilen);
		if (newsockfd < 0)
		{
			error("ERROR on accept");
		}
		ThreadArgs* threadArgs = new ThreadArgs;
		threadArgs->clientSock = newsockfd;

		pthread_t threadID;
		int status = pthread_create(&threadID, NULL, threadFunction, (void*)threadArgs);
		if (status != 0)
		{
			exit(-1);
		}		
	}
	return 0;
}

void* threadFunction(void* args)
{
	struct ThreadArgs* client = (struct ThreadArgs*) args;
	int newsockfd = client->clientSock;

	int n;
	char buffer[256];
	bzero(buffer, 256);

	// new functionality here-------------------------------------------------------------------
	string ssize = to_string(LEADERBOARD_SIZE);
	char const* reallb = ssize.c_str();
	for (long unsigned int i = 0; i < strlen(reallb); i++)
	{
		if (reallb[i] != '\n') buffer[i] = reallb[i];
	}
	n = write(newsockfd, buffer, strlen(buffer));
	if (n < 0)
	{
		//deatch the thread and close socket on error
		printf("ERROR writing to socket\n");
		pthread_detach(pthread_self());
		close(newsockfd);
		return NULL;
	}
	bzero(buffer, 256);

	//read user name from client
	n = read(newsockfd, buffer, 255);
	if (n <= 0)
	{
		//detach the thread and close socket on error
		printf("ERROR reading user name from socket\n");
		pthread_detach(pthread_self());
		close(newsockfd);
		return NULL;
	}
	printf("Here is the user name: %s", buffer);

	//get rid of the newline at the end of the name
	char cname[256];	
	bzero(cname, 256);
	for (long unsigned int i = 0; i < strlen(buffer); i++)
	{
		if (buffer[i] != '\n')
		{
			cname[i] = buffer[i];
		}
	}
	client->clientName = cname;

	//pick random number
	srand(time(NULL));
	int randomNumber = rand() % 999;
	printf("%s's random number is %d \n", cname, randomNumber);
	bool correctGuess = false;
	bzero(buffer, 256);
	
	while (!correctGuess)
	{
		client->turnCounter += 1;
		printf("%s's turn counter is %d\n", cname, client->turnCounter);
		
		int bytesLeft = sizeof(long);
		long networkInt;

		//receive guess from client
		char* bp = (char*)&networkInt;
		while (bytesLeft)
		{
			int bytesRecv = recv(newsockfd, bp, bytesLeft, 0);
			if (bytesRecv <= 0)
			{
				//detach the thread and close socket on error
				printf("ERROR receiving int\n");
				pthread_detach(pthread_self());
				close(newsockfd);
				return NULL;
			}
			bytesLeft = bytesLeft - bytesRecv;
			bp = bp + bytesRecv;
		}
		long hostInt = ntohl(networkInt);
		printf("%s's guess was %ld \n", cname, hostInt);
		bzero(buffer, 256);

		//determine result of the guess
		string s;
		if (hostInt == randomNumber)
		{
			s = "Correct Guess!";
			correctGuess = true;
		}
		else if (hostInt > randomNumber)
		{
			s = "Too high";
		}
		else //if (hostInt < randomNumber)
		{
			s = "Too low";
		}
		for (long unsigned int i = 0; i < s.length(); i++)
		{
			buffer[i] = s[i];
		}

		//send the result of the guess to the client
		n = write(newsockfd, buffer, strlen(buffer));
		if (n < 0)
		{
			//deatch the thread and close socket on error
			printf("ERROR writing to socket\n");
			pthread_detach(pthread_self());
			close(newsockfd);
			return NULL;
		}
	}

	//create victory message that indicates the number of turns it took to guess mystery number
	string temp1 = "Congratulations! It took ";
	string temp2 = to_string(client->turnCounter);
	string temp3;
	if (client->turnCounter == 1)
	{
		temp3 = " turn to guess the number!";
	}
	else
	{
		temp3 = " turns to guess the number!";
	}
	string totalMessage = temp1 + temp2 + temp3;
	bzero(buffer, 256);
	for (long unsigned int i = 0; i < totalMessage.length(); i++)
	{
		buffer[i] = totalMessage[i];
	}
	//send victory message to the client
	n = write(newsockfd, buffer, strlen(buffer));
	if (n < 0)
	{
		//detach the thread and close socket on error
		printf("ERROR writing to socket\n");
		pthread_detach(pthread_self());
		close(newsockfd);
		return NULL;
	}

	//only one thread can update the leaderboard at a time
	sem_wait(&update_mutex);
	printf("Updating leaderboard...\n");
	updateLeaderboard(client->clientName, client->turnCounter);
	sem_post(&update_mutex);

	//make sure if a leaderboard is being sent, that no other threads can update the leaderboard
	sem_wait(&send_mutex);
	client_count++;
	if (client_count == 1)
	{
		sem_wait(&update_mutex);
	}
	sem_post(&send_mutex);

	//send the leaderboard to the client
	printf("Sending leaderboard...\n");
	sendLeaderboard(newsockfd);
	
	//after, if no other threads are also sending the leaderboard, threads can update the leaderboard
	sem_wait(&send_mutex);
	client_count--;
	if (client_count == 0)
	{
		sem_post(&update_mutex);
	}
	sem_post(&send_mutex);

	//detach thread and close socket
	pthread_detach(pthread_self());
	close(newsockfd);
	return NULL;
}

void updateLeaderboard(string name, int count)
{
	int i;
	for (i = LEADERBOARD_SIZE - 1; (i >= 0 && lb[i].turnCounter > count); i--)
	{
		lb[i + 1].clientname = lb[i].clientname;
		lb[i + 1].turnCounter = lb[i].turnCounter;
	}
	if (i != LEADERBOARD_SIZE - 1)
	{
		lb[i + 1].clientname = name;
		lb[i + 1].turnCounter = count;
	}	
}

void sendLeaderboard(int newsockfd)
{
	char buffer[256];
	bzero(buffer, 255);
	int n;
	for (int i = 0; i < LEADERBOARD_SIZE; i++)
	{
		int index = 0;
		bzero(buffer, 256);
		if (lb[i].turnCounter != numeric_limits<int>::max())
		{
			buffer[index] = '\n';
			index++;

			string lbspot = to_string(i + 1) + ". " + lb[i].clientname + " " + to_string(lb[i].turnCounter);
			for (long unsigned int j = 0; j < lbspot.length(); j++)
			{
				buffer[index] = lbspot[j];
				index++;
			}
		}
		n = write(newsockfd, buffer, strlen(buffer));
		if (n < 0)
		{
			printf("ERROR sending leaderboard");
			return;
		}
	}
}
/*
bool sendUInt32(int sock, uint32_t val)
{
	int value = htonl(val);
	return sendAll(sock, &value, sizeof(value));
}
*/