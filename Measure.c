/*
	TCP/IP-server
*/

#include<stdio.h>
#include <time.h>

#if defined _WIN32
#include<winsock2.h>   //winsock2 should be before windows
#pragma comment(lib,"ws2_32.lib")
#include <ws2tcpip.h>
#else
// Linux and other UNIXes
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
#endif

#define SERVER_PORT 5060  //The port that the server listens

void changeCC(int sock)
{
	char* buf = "reno";
	socklen_t len = strlen(buf);
	if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0) {
		perror("setsockopt");
		return -1;
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
#else
	signal(SIGPIPE, SIG_IGN); // on linux to prevent crash on closing socket
#endif

	// Open the listening (server) socket
	int listeningSocket = -1;

	if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Could not create listening socket : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
		);
	}

	// Reuse the address if the server socket on was closed
	// and remains for 45 seconds in TIME-WAIT state till the final removal.
	//
	int enableReuse = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR,
#if defined _WIN32
	(const char*)
#endif
		& enableReuse,

		sizeof(int)) < 0)
	{
		printf("setsockopt() failed with error code : %d",
#if defined _WIN32
			WSAGetLastError()
#else
			errno
#endif
		);
	}

	// "sockaddr_in" is the "derived" from sockaddr structure
	// used for IPv4 communication. For IPv6, use sockaddr_in6
	//
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(SERVER_PORT);  //network order

	// Bind the socket to the port with any IP at this port
	if (bind(listeningSocket, (struct sockaddr*) & serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("Bind failed with error code : %d",
#if defined _WIN32
			WSAGetLastError()
#else
			errno
#endif
		);
		// TODO: close the socket
		return -1;
	}

	printf("Bind() success\n");

	// Make the socket listening; actually mother of all client sockets.
	if (listen(listeningSocket, 500) == -1) //500 is a Maximum size of queue connection requests
											//number of concurrent connections 
	{
		printf("listen() failed with error code : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
		);
		// TODO: close the socket
		return -1;
	}

	//Accept and incoming connection
	printf("Waiting for incoming TCP-connections...\n");

	struct sockaddr_in clientAddress;  //
	socklen_t clientAddressLen = sizeof(clientAddress);

	while (1)
	{
		memset(&clientAddress, 0, sizeof(clientAddress));
		clientAddressLen = sizeof(clientAddress);
		int clientSocket = accept(listeningSocket, (struct sockaddr*) & clientAddress, &clientAddressLen);
		if (clientSocket == -1)
		{
			printf("listen failed with error code : %d"
#if defined _WIN32
				, WSAGetLastError()
#else
				, errno
#endif
			);
			// TODO: close the sockets
			return -1;
		}

		printf("A new client connection accepted\n");

		double time[2][5];
		int algoCount = 0, loopsCount = 0;
		while (algoCount < 2)
		{
			int readSize;
			char* recvMssg;
			struct timeval start, stop;
			readSize = recv(clientSocket, recvMssg, 2048, 0);
			while (readSize = 0)
			{
				readSize = recv(clientSocket, recvMssg, 2048, 0);
			}
			gettimeofday(&start, 0);
			while (strcmp(recvMssg, "End of file") && readSize > 0)
			{
				readSize = recv(clientSocket, recvMssg, 2048, 0);
			}
			gettimeofday(&stop, 0);
			time[algoCount][loopsCount] = timedifference_msec(start, stop);
			loopsCount++;
			if (loopsCount == 5)
			{
				double avg = getAvg(time[algoCount], loopsCount);
				loopsCount = 0;
				algoCount++;
				char* cong[256];
				getCong(cong, clientSocket);
				printf("avg of %s: %d (milSec)", cong, avg);
				changeCC(clientSocket);
			}
		}

	}

	// TODO: All open clientSocket descriptors should be kept
	// in some container and closed as well.
#if defined _WIN32
	closesocket(listeningSocket);
	WSACleanup();
#else
	close(listeningSocket);
#endif

	return 0;
}
