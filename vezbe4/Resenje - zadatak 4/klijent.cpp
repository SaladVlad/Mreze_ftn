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

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 19010
#define BUFFER_SIZE 256

// TCP client that use blocking sockets
int main()
{
    // Socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;

    // Variable used to store function return value
    int iResult;

    // Buffer we will use to store message
    char dataBuffer[BUFFER_SIZE];

    // WSADATA data structure that is to receive details of the Windows Sockets implementation
    WSADATA wsaData;

    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;								// IPv4 protocol
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
    serverAddress.sin_port = htons(SERVER_PORT);					// server port

    // Connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    int nizBrojeva[3];  //niz brojeva koji saljemo

    while (true)
    {
        // Ucitavanje niza od 3 broja
        for (int i = 0; i < 3; i++)
        {
            printf("\n Unesite broj za slanje: ");
            gets_s(dataBuffer, BUFFER_SIZE);
            nizBrojeva[i] = atoi(dataBuffer);
            nizBrojeva[i] = htonl(nizBrojeva[i]);
        }
        
        // saljemo nizBrojeva prosledjujuci njegovu adresu i duzinu poruke (niza) u bajtima
        iResult = send(connectSocket, (char*)nizBrojeva, 3 * sizeof(int), 0);

        // Check result of send function
        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
        printf("Message successfully sent. Total bytes: %ld\n", iResult);

        //prijem poruke koju salje server
        iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);

        if (iResult > 0)	// Check if message is successfully received
        {
            dataBuffer[iResult] = '\0';
            printf("\n Server salje: %s\n", dataBuffer);
        }
        else if (iResult == 0)	// Check if shutdown command is received
        {
            // Connection was closed successfully
            printf("Connection with server closed.\n");
            break;
        }
        else	// greska pri prijemu
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }

        printf(" \nZa prekid slanja unite 'exit', a za nastavak pritisnite bilo koji taster");
        gets_s(dataBuffer, BUFFER_SIZE);
        if (!strcmp(dataBuffer, "exit"))
            break;
    }
    // Shutdown the connection since we're done
    iResult = shutdown(connectSocket, SD_BOTH);

    // Check if connection is succesfully shut down.
    if (iResult == SOCKET_ERROR)
    {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // For demonstration purpose
    printf("\nPress any key to exit: ");
    _getch();


    // Close connected socket
    closesocket(connectSocket);

    // Deinitialize WSA library
    WSACleanup();

    return 0;
}