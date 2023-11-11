#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


int main()
{
	// Initialize Winsock, request the Winsock 2.2
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup().\n");
		return 1;
	}
	else
	{
		printf("WSA library is succesfully initilized.\n");
	}

	// Create a socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("Error at socket().\n");
		WSACleanup();
		return 1;
	}
	else
	{
		printf("\nSocket is succesfully created.\n");
	}

	// Get and print local ip address and host name
	hostent* hostDetails = gethostbyname("");

	struct in_addr hostAddr = *(struct in_addr*)hostDetails->h_addr_list[0];
	char* ip = inet_ntoa(hostAddr);

	printf("\nIP address: %s.\n", ip);
	printf("Hostname: %s.\n", hostDetails->h_name);


	// Bind the socket to the local IP address and application port
	unsigned short port = 5555; // try also with value 80

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(ip); //try also with ADDR_ANY
	serverAddress.sin_port = htons(port);

	if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		printf("bind() failed. Error: %d\n", errorCode);

		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("\nSocket is succesfully binded to address.\n");
	}

	// Checkout socket address and port
	struct sockaddr_in sockAddrStruct;
	int sockAddrLen = sizeof(sockAddrStruct);

	getsockname(serverSocket, (struct sockaddr*)&sockAddrStruct, &sockAddrLen);

	unsigned short sockPort = ntohs(sockAddrStruct.sin_port);
	char* sockAddress = inet_ntoa(sockAddrStruct.sin_addr);

	printf("\nSocket IP address: %s\n", sockAddress);
	printf("Socket port: %d \n\n", sockPort);

	// Close socket and finish work with Winsock library
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}