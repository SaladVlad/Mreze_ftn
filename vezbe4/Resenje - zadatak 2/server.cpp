// UDP server that use blocking sockets
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

#define SERVER_PORT1 17010  // Port prve serverske uticnice
#define SERVER_PORT2 17011	// Port druge serverske uticnice

#define BUFFER_SIZE 512		// Size of buffer that will be used for sending and receiving messages to clients

int main()
{
    // Dve adresne strukture za svaku serversku uticnicu
    sockaddr_in serverAddress1, serverAddress2;

    // Buffer we will use to send and receive clients' messages
    char dataBuffer[BUFFER_SIZE];

    // WSADATA data structure that is to receive details of the Windows Sockets implementation
    WSADATA wsaData;

    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    //korak 1. Popunjavanje adresnih struktura, kreiranje dve uticnice i njihovo povezivanje 
    //sa adresnim podacima u tim strukturama

    // Initialize serverAddress1 and serverAddress2 structures used by bind function
    memset((char*)&serverAddress1, 0, sizeof(serverAddress1));
    serverAddress1.sin_family = AF_INET; 	// set server address protocol family
    serverAddress1.sin_addr.s_addr = inet_addr("127.0.0.1");	// loopback lokalna adresa na sopstveni racunar
    serverAddress1.sin_port = htons(SERVER_PORT1);	// 17010 port

    memset((char*)&serverAddress2, 0, sizeof(serverAddress2));
    serverAddress2.sin_family = AF_INET; 	// set server address protocol family
    serverAddress2.sin_addr.s_addr = INADDR_ANY;	// sve dostupne adrese na datom racunaru
    serverAddress2.sin_port = htons(SERVER_PORT2);	// 17011 port

    // Create first socket
    SOCKET serverSocket1 = socket(AF_INET,      // IPv4 address famly
        SOCK_DGRAM,   // datagram socket
        IPPROTO_UDP); // UDP

    // Check if socket creation succeeded
    if (serverSocket1 == INVALID_SOCKET)
    {
        printf("Creating socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Create second socket
    SOCKET serverSocket2 = socket(AF_INET,      // IPv4 address famly
        SOCK_DGRAM,   // datagram socket
        IPPROTO_UDP); // UDP

    // Check if socket creation succeeded
    if (serverSocket2 == INVALID_SOCKET)
    {
        printf("Creating socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Povezujemo 1. uticnicu sa njenom adresnom strukturom 
    int iResult = bind(serverSocket1, (SOCKADDR*)&serverAddress1, sizeof(serverAddress1));

    // Check if socket is succesfully binded to server datas
    if (iResult == SOCKET_ERROR)
    {
        printf("Socket bind failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket1);
        WSACleanup();
        return 1;
    }

    // Povezujemo 2. uticnicu sa njenom adresnom strukturom 
    iResult = bind(serverSocket2, (SOCKADDR*)&serverAddress2, sizeof(serverAddress2));

    // Check if socket is succesfully binded to server address data
    if (iResult == SOCKET_ERROR)
    {
        printf("Socket bind failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket2);
        WSACleanup();
        return 1;
    }

    printf("Simple UDP server started and waiting client messages.\n");

    // Declare client address that will be set from recvfrom
    sockaddr_in clientAddress;
    // size of client address structure
    int sockAddrLen = sizeof(clientAddress);

    // Korak 2.a Stavljamo uticnice u neblokirajuci rezim
    unsigned long mode = 1;
    if (ioctlsocket(serverSocket1, FIONBIO, &mode) != 0 || ioctlsocket(serverSocket2, FIONBIO, &mode) != 0)
    {
        printf("ioctlsocket failed with error %d\n", WSAGetLastError());
        closesocket(serverSocket1);
        closesocket(serverSocket2);
        WSACleanup();
        return 1;
    }

    
    
    //pomocne promenljive za 3. korak
    bool primljen1 = false, primljen2 = false; //detekcija pristigle poruke
  
    int poeni1 = 0, poeni2 = 0;  //brojac poena za prvog i drugog klijenta
   
    //Korak 2. Polling model prijema poruka na ove dve uticnice
    while (1)
    {
        // initialize client address structure in memory
        memset(&clientAddress, 0, sizeof(clientAddress));

        // Set whole buffer to zero
        memset(dataBuffer, 0, BUFFER_SIZE);

        // Prijem poruke sa uticnici br. 1
        iResult = recvfrom(serverSocket1,				// Own socket
            dataBuffer,					// Buffer that will be used for receiving message
            BUFFER_SIZE,					// Maximal size of buffer
            0,							// No flags
            (SOCKADDR*)&clientAddress,	// Client information from received message (ip address and port)
            &sockAddrLen);				// Size of sockadd_in structure


        // Provera da li je uspesno primljena poruka
        if (iResult != SOCKET_ERROR)
        {
            // Set end of string
            dataBuffer[iResult] = '\0';

            //ispis primljene poruke na ekran
            printf("Serverska uticnica br. 1 je primila: %s.\n", dataBuffer);

            primljen1 = true;  //prvi klijent je poslao poruku i ona je primljena
            
            if (primljen1 && !primljen2) //ako je stigla poruka od 1. klijenta, ali ne i od 2. klijenta
            {
                poeni1++;
                printf("Prvi klijent je prvi poslao poruku i ima %d poena.\n", poeni1);
            }
        }
        else  //iResult == SOCKET_ERROR tj. desila se greska
        {
            //obavezno proveriti da li je operacija vratila WSAEWOULDBLOCK
            //to oznacava da bi operacija recvfrom() blokirala programa
            //jer jos nije spremna da se izvrsi tj. poruka nije stigla
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                //pusticemo da program tece dalje jer imamo pokusaj 
                //prijema poruke i na 2. uticnici, tamo cemo pozvati Sleep
            }
            else {
                // Desila se neka druga greska prilikom prijema poruke
                //izaci cemo iz petlje, zatvoriti uticnice i ugasiti program
                break;

            }

        }

        // Prijem poruke sa uticnici br. 2
        iResult = recvfrom(serverSocket2,				// Own socket
            dataBuffer,					// Buffer that will be used for receiving message
            BUFFER_SIZE,					// Maximal size of buffer
            0,							// No flags
            (SOCKADDR*)&clientAddress,	// Client information from received message (ip address and port)
            &sockAddrLen);				// Size of sockadd_in structure


        // Provera da li je uspesno primljena poruka
        if (iResult != SOCKET_ERROR)
        {
            // Set end of string
            dataBuffer[iResult] = '\0';

            //ispis primljene poruke na ekran
            printf("Serverska uticnica br. 2 je primila: %s.\n", dataBuffer);
            primljen2 = true; //drugi klijent je poslao poruku i ona je primljena
            
            if (!primljen1 && primljen2) //ako je stigla poruka od 2. klijenta, ali ne i od 1. klijenta
            {
                poeni2++;
                printf("Drugi klijent je prvi poslao poruku i ima %d poena.\n", poeni2);
            }
        }
        else  //iResult == SOCKET_ERROR tj. desila se greska
        {
            //obavezno proveriti da li je operacija vratila WSAEWOULDBLOCK
            //to oznacava da bi operacija recvfrom() blokirala programa
            //jer jos nije spremna da se izvrsi tj. poruka nije stigla
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                Sleep(1500); //korak 2.b cekamo do narednog pokusaja prijema poruke 1.5s
            }
            else {
                // Desila se neka druga greska prilikom prijema poruke
                //izaci cemo iz petlje, zatvoriti uticnice i ugasiti program
                break;

            }

        }

        if (primljen1 && primljen2) //ako su stigle poruke od oba klijenta, za naredni ciklus provere resetujemo promenjlive 
        {
            primljen1 = false;
            primljen2 = false;
        }

    }

    // Close server application
    iResult = closesocket(serverSocket1);
    if (iResult == SOCKET_ERROR)
    {
        printf("closesocket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    iResult = closesocket(serverSocket2);
    if (iResult == SOCKET_ERROR)
    {
        printf("closesocket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Server successfully shut down.\n");

    // Close Winsock library
    WSACleanup();

    return 0;
}