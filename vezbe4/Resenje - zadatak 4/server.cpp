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

#define SERVER_PORT 19010
#define BUFFER_SIZE 256

// TCP server that use blocking sockets
int main()
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;

    // Socket used for communication with client
    SOCKET acceptedSocket[2];
    acceptedSocket[0] = INVALID_SOCKET;
    acceptedSocket[1] = INVALID_SOCKET;

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

    //korak 1.a stavljanje uticnice listenSocket u neblokirajuci rezim
    unsigned long mode = 1; //non-blocking mode
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR)
        printf("ioctlsocket failed with error: %ld\n", iResult);
    
    printf("Server socket is set to listening mode. Waiting for new connection requests.\n");

    int connected = 0; //brojac koliko klijenata je ostvarilo vezu sa serverom
    do
    {
        //prijem konekcija u neblokirajucem rezimu, korak 1
        
        // Struct for information about connected client
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(struct sockaddr_in);
        
        //pozivamo accept() za prijem konekcije 
        acceptedSocket[connected] = accept(listenSocket, (struct sockaddr*) & clientAddr, &clientAddrSize);
        // proveramo da li se accept() funkcija uspesno izvrsila
        if (acceptedSocket[connected] != INVALID_SOCKET)
        {
            //korak 1.c
            printf("\nNew client request accepted. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            //korak 2.a postavljanje uticnica namenjenih klijentima u neblokirajuci rezim
            unsigned long mode = 1; //non-blocking mode
            iResult = ioctlsocket(acceptedSocket[connected], FIONBIO, &mode);
            if (iResult != NO_ERROR)
                printf("ioctlsocket failed with error: %ld\n", iResult);

            connected++; //uvecavamo brojac konektovanih klijenata

        }
        else // funkcija je vratila INVALID_SOCKET
        {
            //obavezno proveriti da li je razlog WSAEWOULDBLOCK
            //tj. zahtev za vezom jos nije stigao
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                Sleep(2000);
            }
            else  //ili je neka druga greska zbog koje cemo ugasiti server
            {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(listenSocket);
                WSACleanup();
                return 1;
            }

        }

        if (connected < 2)  //vracamo se na pocetak petlje dok se ne konektuju 2 klijenta
        {
            continue;
        }
        
       //kad se konektuju 2 klijenta program nastavlje dalje
        //ocekuje se razmena poruka sa ta dva klijenta

        int* primljenNiz; //pokazivac na primljen niz celobrojnih vrednosti
        int max = 0; // maksimalni (najveci) broj iz primljenog niza
        
        //korak 2. polling model prijema poruka
        do
        {
            // For petljom prolazimo kroz niz acceptedSocket od dve uticnice 
            //i proveravamo da li su primile poruku primenom polling modela 
            for (int i = 0; i < 2; i++)
            {
                iResult = recv(acceptedSocket[i], dataBuffer, BUFFER_SIZE, 0);

                if (iResult > 0)	// poruka je uspesno primljena
                {
                   // pristupamo adresi gde je smestena primljena poruka, adekvatno kastujemo pokazivac
                    primljenNiz = (int*)dataBuffer;
                    
                    printf("Klijent br. [%d] je poslao: %d %d %d.\n", i+1, ntohl(primljenNiz[0]), ntohl(primljenNiz[1]), ntohl(primljenNiz[2]));
                    
                    //pronalazenje najvece vrednosti, inicijalno uzimamo prvi element niza
                    max = ntohl(primljenNiz[0]); //kad citamo iz poruke (sadrzaj je u mreznom formatu), pa nam treba ntohl()
                    for (int i = 1; i < 3; i++)
                    {
                        if (ntohl(primljenNiz[i])> max)  
                            max = ntohl(primljenNiz[i]);  //ovo je novi najveci element niza

                    }
                     
                    //priprema poruke o pronadjenoj max vrednosti 
                    sprintf_s(dataBuffer, "Najveci poslati broj je %d.", max);
                    //slanje poruke ka klijentu
                    iResult = send(acceptedSocket[i], dataBuffer, strlen(dataBuffer), 0);

                    // Check result of send function
                    if (iResult == SOCKET_ERROR)
                    {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(acceptedSocket[i]);
                        connected--;
                        break;
                    }
                    
                }
                else if (iResult == 0)	// Check if shutdown command is received
                {
                    // Connection was closed successfully
                    printf("Connection with client closed.\n");
                    closesocket(acceptedSocket[i]);
                    connected--;
                    break;
                }
                else	// u neblokir. rezimu funkcija se cesto neuspesno izvrsi jer nije spremna, pa bi zelela da blokira program
                {
                    if (WSAGetLastError() == WSAEWOULDBLOCK) {
                        // U pitanju je blokirajuca operacija 
                        // tj. poruka jos nije primljena
                    }
                    else {
                        // Desila se neka druga greska prilikom poziva operacije
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(acceptedSocket[i]);
                        connected--;
                        break;
                    }

                }
                
            }
           
            Sleep(1000); //kad smo proverili na obe uticnice do naredne iteracije pauziramo 1s
            if (connected < 2) //ako se desila greska i jedna ili obe uticnice  su zatvorene prekidamo petlju prijema poruka 
                break;
        } while (true);

        connected = 0; //resetujemo brojac konektovanih za nova dva klijenta sa kojima ce se ostvariti veza

    } while (true);


    //Close listen and accepted sockets
    closesocket(listenSocket);
    closesocket(acceptedSocket[0]);
    closesocket(acceptedSocket[1]);

    // Deinitialize WSA library
    WSACleanup();

    return 0;
}