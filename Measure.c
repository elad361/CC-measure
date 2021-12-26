/*
	Author: Elad Shoham
	ID: 205439649 
*/

#include<stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h> 

#define SERVER_PORT 5060  //The port that the server listens

void changeCC(int sock)
{
	char* buf = "reno";
	socklen_t len = strlen(buf);
	if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
		perror("setsockopt");
		exit(1);
	}
}

double getAvg(double* times, int num)
{
	double sum = 0;
	for (int i = 0; i < num; i++)
	{
		sum += times[i];
	}
	return (double)(sum / num);
}

void getCong(char* cong, int sock)
{
	socklen_t len = sizeof(cong);
	if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, cong, &len) != 0) {
		perror("getsockopt");
		exit(1);
	}
}

int main()
{
	signal(SIGPIPE, SIG_IGN); // on linux to prevent crash on closing socket

	// Open the listening (server) socket
	int listeningSocket = -1;

	if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Could not create listening socket : %d", errno);
	}

	// Reuse the address if the server socket on was closed
	// and remains for 45 seconds in TIME-WAIT state till the final removal.
	
	int enableReuse = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, & enableReuse, sizeof(int)) < 0)
	{
		printf("setsockopt() failed with error code : %d", errno);
	}

	// "sockaddr_in" is the "derived" from sockaddr structure
	// used for IPv4 communication. For IPv6, use sockaddr_in6
	
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(SERVER_PORT);  //network order

	// Bind the socket to the port with any IP at this port
	if (bind(listeningSocket, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("Bind failed with error code : %d", errno);
		close(listeningSocket);
		return -1;
	}

	printf("Bind() success\n");

	// Make the socket listening; actually mother of all client sockets.
	if (listen(listeningSocket, 1) == -1)  // 1 is a Maximum size of queue connection requests
	{
		printf("listen() failed with error code : %d", errno);
		close(listeningSocket);
		return -1;
	}

	// Accept and wait for incoming connection
	printf("Waiting for incoming TCP-connections...\n");

	struct sockaddr_in clientAddress;  //
	socklen_t clientAddressLen = sizeof(clientAddress);
	memset(&clientAddress, 0, sizeof(clientAddress));
	clientAddressLen = sizeof(clientAddress);
	int clientSocket = accept(listeningSocket, (struct sockaddr*) & clientAddress, &clientAddressLen);
	if (clientSocket == -1)
	{
		printf("listen failed with error code : %d", errno);
		close(clientSocket);
		close(listeningSocket);
		return -1;
	}

	printf("A new client connection accepted\n");

	double time[2][5];
	int algoCount = 0, loopsCount = 0;
	while (algoCount < 2)
	{
		int readSize = 0, totalbytes = 0;
		char recvMsg[1024+1];
		struct timeval start, stop;
		double start_ms, stop_ms;
		while (readSize == 0)
		{
			readSize = recv(clientSocket, recvMsg, 1024, 0);
		}
		recvMsg[readSize] = 0;
		gettimeofday(&start, NULL);
		start_ms = (start.tv_sec * 1000) + (start.tv_usec / 1000);
		//printf("start_ms %f\n", start_ms);
		totalbytes += readSize;
		while (!(strstr(recvMsg, "end")))
		{
			readSize = recv(clientSocket, recvMsg, 1024, 0);
			recvMsg[readSize] = 0;
			totalbytes += readSize;
		}
		gettimeofday(&stop, NULL);
		stop_ms = (stop.tv_sec * 1000) + (stop.tv_usec / 1000);
		//printf("stop_ms %f\n", stop_ms);
		time[algoCount][loopsCount] = stop_ms - start_ms;
		printf("File num: %d, Bytes received: %d, Time: %.2f ms\n", loopsCount + 1, totalbytes, time[algoCount][loopsCount]);
		loopsCount++;
		if (loopsCount == 5)
		{
			double avg = getAvg(time[algoCount], loopsCount);
			loopsCount = 0;
			algoCount++;
			char cong[256];
			getCong(cong, clientSocket);
			printf("*** Avg of %s ***: %.2f ms\n", cong, avg);
			changeCC(clientSocket);
		}
	}

	close(clientSocket);	
	close(listeningSocket);

	return 0;
}
