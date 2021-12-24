/*
	Author: Elad Shoham
	ID: 205439649 
*/


#include <stdio.h>
#include <string.h> 


#if defined _WIN32

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <netinet/tcp.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "172.17.17.23"
#define NUM_OF_LOOPS 5
#define NUM_OF_ALGO 2

void sendFileThrueTCP(FILE *file, int sock)
{
	char data[1024] = { 0 };
	while (fgets(data, 1024, file) != NULL)
	{
		int bytesSent = send(sock, data, strlen(data) + 1, 0);

		if (-1 == bytesSent)
		{
			printf("send() failed with error code : %d"
#if defined _WIN32
				, WSAGetLastError()
#else
				, errno
#endif
			);
		}
		else if (0 == bytesSent)
		{
			printf("peer has closed the TCP connection prior to send().\n");
		}
		else if (strlen(data) > bytesSent)
		{
			int sent = strlen(data);
			printf("sent only %d bytes from the required %d.\n", bytesSent, sent);
		}
	}
}

void changeCC(int sock)
{
	char* buf = "reno";
	socklen_t len = strlen(buf);
	if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
		perror("setsockopt");
		return -1;
	}
}

int main()
{
#if defined _WIN32
	// Windows requires initialization
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
#endif

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1)
	{
		printf("Could not create socket : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
		);
	}

	// "sockaddr_in" is the "derived" from sockaddr structure
	// used for IPv4 communication. For IPv6, use sockaddr_in6
	//
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
		return -1;
	}

	// Make a connection to the server with socket SendingSocket.

	if (connect(sock, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("connect() failed with error code : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
		);
	}

	printf("connected to server\n");

	char* fileName = "1gb.txt";
	FILE* file = fopen(fileName, "r");
	if (file == NULL)
	{
		printf("failed to read file: %d", fileName);
		exit(1);
	}
	int algoCount = 0, loopsCount = 0;
	while (algoCount < NUM_OF_ALGO)
	{
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


	// TODO: All open clientSocket descriptors should be kept
		// in some container and closed as well.
#if defined _WIN32
	closesocket(sock);
	WSACleanup();
#else
	close(sock);
#endif

	return 0;
}
