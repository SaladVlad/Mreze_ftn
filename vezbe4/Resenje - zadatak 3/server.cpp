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

#define SERVER_PORT 18010
#define BUFFER_SIZE 256

// TCP server that use blocking sockets
int main()
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;

    // Socket used for communication with client
    SOCKET acceptedSocket1 = INVALID_SOCKET;
    SOCKET acceptedSocket2 = INVALID_SOCKET;

    // Variable used to store function return value
    int iResult;

    // Buffer used for storing incoming data
    char dataBuffer[BUFFER_SIZE];

    // WSADATA data structure that is to receive details of the Windows Sockets implementation
    WSADATA wsaData;

    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }


    // Initialize serverAddress structure used by bind
    sockaddr_in serverAddress;
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;				// IPv4 address family
    serverAddress.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses
    serverAddress.sin_port = htons(SERVER_PORT);	// Use specific port


    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address family
        SOCK_STREAM,  // Stream socket
        IPPROTO_TCP); // TCP protocol

    // Check if socket is successfully created
    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    iResult = bind(listenSocket, (struct sockaddr*) & serverAddress, sizeof(serverAddress));

    // Check if socket is successfully binded to address and port from sockaddr_in structure
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server socket is set to listening mode. Waiting for new connection requests.\n");

    do
    {
        // Struct for information about connected client
        sockaddr_in clientAddr;

        int clientAddrSize = sizeof(struct sockaddr_in);

        // Prihvat veze sa 1. klijentom 
        acceptedSocket1 = accept(listenSocket, (struct sockaddr*) & clientAddr, &clientAddrSize);

        // Check if accepted socket is valid 
        if (acceptedSocket1 == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        printf("\nNew client request accepted. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // Prihvat veze sa 2. klijentom 
        acceptedSocket2 = accept(listenSocket, (struct sockaddr*) & clientAddr, &clientAddrSize);

        // Check if accepted socket is valid 
        if (acceptedSocket2 == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        printf("\nNew client request accepted. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));


        //korak 2.a postavljanje uticnica namenjenih klijentima u neblokirajuci rezim
        unsigned long mode = 1; //non-blocking mode
        iResult = ioctlsocket(acceptedSocket1, FIONBIO, &mode);
        if (iResult != NO_ERROR)
            printf("ioctlsocket failed with error: %ld\n", iResult);

        iResult = ioctlsocket(acceptedSocket2, FIONBIO, &mode);
        if (iResult != NO_ERROR)
            printf("ioctlsocket failed with error: %ld\n", iResult);


        int* primljenNiz;
        int suma = 0;
        //polling model prijema poruka
        do
        {
            // Prijem poruke na 1. uticnici
            iResult = recv(acceptedSocket1, dataBuffer, BUFFER_SIZE, 0);

            if (iResult > 0)	// Check if message is successfully received
            {
                //dataBuffer[iResult] = '\0';
                primljenNiz = (int*)dataBuffer;
                // printf("Klijent br. 1 je poslao: %s.\n", dataBuffer);
                
                printf("Klijent br. 1 je poslao: %d %d %d.\n", ntohl(primljenNiz[0]), ntohl(primljenNiz[1]), ntohl(primljenNiz[2]));
                for (int i = 0; i < 3; i++)
                    suma += ntohl(primljenNiz[i]); //kad citamo iz poruke (sadrzaj je u mreznom formatu), pa nam treba ntohl()

                //priprema podatka za slanje, treba nam konverzija iz host u network zapis tj. htonl()
                suma = htonl(suma);
                //slanje poruke ka 1. klijentu
                iResult = send(acceptedSocket1, (char*)&suma, sizeof(int), 0);

                // Check result of send function
                if (iResult == SOCKET_ERROR)
                {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(acceptedSocket1);
                    break;
                }
                suma = 0; //ponovno postavljanje na 0, za naredno izracunavanje nove sume brojeva

            }
            else if (iResult == 0)	// Check if shutdown command is received
            {
                // Connection was closed successfully
                printf("Connection with client closed.\n");
                closesocket(acceptedSocket1);
                break;
            }
            else	// u neblokir. rezimu funkcija se cesto neuspesno iyvrsi jer nije spreman, pa bi zelela da blokira program
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    // U pitanju je blokirajuca operacija 
                    // tj. poruka jos nije stigla
                }
                else {
                    // Desila se neka druga greska prilikom poziva operacije
                    printf("recv failed with error: %d\n", WSAGetLastError());
                    closesocket(acceptedSocket1);
                    closesocket(acceptedSocket2);
                    break;
                }

            }

            // Prijem poruke na 2. uticnici
            iResult = recv(acceptedSocket2, dataBuffer, BUFFER_SIZE, 0);

            if (iResult > 0)	// Check if message is successfully received
            {
                primljenNiz = (int*)dataBuffer;
                // printf("Klijent br. 2 je poslao: %s.\n", dataBuffer);
                
                printf("Klijent br. 2 je poslao: %d %d %d.\n", ntohl(primljenNiz[0]), ntohl(primljenNiz[1]), ntohl(primljenNiz[2]));

                for (int i = 0; i < 3; i++)
                    suma += ntohl(primljenNiz[i]); //kad citamo iz poruke (sadrzaj je u mreznom formatu), pa nam treba ntohl()

                //priprema podatka za slanje, treba nam konverzija iz host u network zapis tj. htonl()
                suma = htonl(suma);
                //slanje ka 2. klijentu
                iResult = send(acceptedSocket2, (char*)&suma, sizeof(int), 0);

                // Check result of send function
                if (iResult == SOCKET_ERROR)
                {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(acceptedSocket2);
                    break;
                }
                suma = 0;
            }
            else if (iResult == 0)	// Check if shutdown command is received
            {
                // Connection was closed successfully
                printf("Connection with client closed.\n");
                closesocket(acceptedSocket2);
                break;
            }
            else	// u neblokir. rezimu funkcija se cesto neuspesno iyvrsi jer nije spreman, pa bi zelela da blokira program
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    // U pitanju je blokirajuca operacija 
                    // tj. poruka jos nije stigla
                    Sleep(1500);
                }
                else {
                    // Desila se neka druga greska prilikom poziva operacije
                    printf("recv failed with error: %d\n", WSAGetLastError());
                    closesocket(acceptedSocket2);
                    closesocket(acceptedSocket1);
                    break;
                }

            }

        } while (true);



    } while (true);


    //Close listen and accepted sockets
    closesocket(listenSocket);
    closesocket(acceptedSocket1);
    closesocket(acceptedSocket2);

    // Deinitialize WSA library
    WSACleanup();

    return 0;
}