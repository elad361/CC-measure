/*
	Author: Elad Shoham
	ID: 205439649 
*/


#include <stdio.h>
#include <string.h> 
#include <netinet/tcp.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"
#define NUM_OF_LOOPS 5
#define NUM_OF_ALGO 2
#define FILE_BUFF 1024+1

void sendFileThrueTCP(FILE *file, int sock)
{
	int total_sent = 0;
	char data[FILE_BUFF] = { 0 };
	while (fgets(data, FILE_BUFF, file) != NULL)
	{
		int bytesSent = send(sock, data, strlen(data), 0);

		total_sent += bytesSent;

		if (-1 == bytesSent)
		{
			printf("send() failed with error code : %d"	, errno);
		}
		else if (0 == bytesSent)
		{
			printf("Peer has closed the TCP connection prior to send().\n");
		}
		else if (strlen(data) > bytesSent)
		{
			int sent = strlen(data);
			printf("Sent only %d bytes from the required %d.\n", bytesSent, sent);
		}
		else
		{
			//printf("bytes sent: %d, total sent: %d\n", bytesSent, total_sent);
		}
	}

	strcpy(data, "end");
	int bytesSent = send(sock, data, strlen(data), 0);
	total_sent += bytesSent;
	printf("Total sent: %d\n", total_sent);
	sleep(1);
}

void changeCC(int sock)
{
	char* buf = "reno";
	socklen_t len = strlen(buf);
	if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
		perror("setsockopt");
		exit(1);
	}
}

int main()
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1)
	{
		printf("Could not create socket : %d", errno);
		return -1;
	}
	else 
	{
		printf("Socket %d created\n", sock);
	}

	// "sockaddr_in" is the "derived" from sockaddr structure
	// used for IPv4 communication. For IPv6, use sockaddr_in6
	
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
		close(sock);
		return -1;
	}

	// Make a connection to the server with socket SendingSocket.
	printf("Trying to connect...\n");
	if (connect(sock, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("connect() failed with error code : %d", errno);
		close(sock);
		return -1;
	}

	printf("Connected to server\n");

	char* fileName = "1gb.txt";
	
	int algoCount = 0, loopsCount = 0;
	while (algoCount < NUM_OF_ALGO)
	{
		printf("%d: Start sending file %d\n", algoCount + 1, loopsCount + 1);
		FILE* file = fopen(fileName, "r");
		if (file == NULL)
		{
			printf("Failed to read file: %s", fileName);
			close(sock);
			return -1;
		}

		sendFileThrueTCP(file, sock);

		loopsCount++;
		if (loopsCount == 5)
		{
			changeCC(sock);
			loopsCount = 0;
			algoCount++;
		}
		fclose(file);
	}

	close(sock);
	return 0;
}
