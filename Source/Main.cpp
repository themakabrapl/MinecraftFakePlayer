#include <iostream>
#include <string>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <tchar.h>

#include <rpc.h>

#include "Ndt.h"

int main()
{
	int wsaerr;
	unsigned short port = 25565;
	std::string serverAddress = "127.0.0.1";

	/* This Part of the Program was made while learning Winsock
		and 100 % of this code was written by Nicholas Day on https ://www.youtube.com/c/NicholasDayPhD

	Link to the amazing tutorial that he made : https://www.youtube.com/watch?v=gntyAFoZp-E */

	//Setting up a Client Socket
	SOCKET cSocket;

	//Createing a Data Structure for information about Winsock Implementation
	WSADATA wsaData;

	//Specifying the version of Winsock
	WORD wVersionRequested = MAKEWORD(2, 2);

	//Initializing Winsock 
	wsaerr = WSAStartup(wVersionRequested, &wsaData);

	if (wsaerr != 0)
	{
		std::cout << "The Winsock dll not found" << std::endl;
		return 0;
	}
	std::cout << "The Winsock dll found" << std::endl;
	std::cout << "The Status:" << wsaData.szSystemStatus << std::endl;

	//Setting up client Socket
	cSocket = INVALID_SOCKET;
	cSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (cSocket == INVALID_SOCKET)
	{
		std::cout << "Error at Socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}
	std::cout << "Socket is OK" << std::endl;

	//Createing a Data Structure for information about ip address and port to connect to
	sockaddr_in addrpo;
	addrpo.sin_family = AF_INET;
	InetPton(AF_INET, _T("127.0.0.1"), &addrpo.sin_addr.s_addr);
	addrpo.sin_port = htons(port);

	if (connect(cSocket, (SOCKADDR*)&addrpo, sizeof(addrpo)) == SOCKET_ERROR)
	{
		std::cout << "Connect() failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}
	std::cout << "Connect() is OK" << std::endl;

	/* End of Nicholas's Code*/

	//Setting up variables used in communication
	int byteCount = 0;
	int packetSize = 0;
	int packetsSent = 0;
	int portSize = sizeof(port);
	char* packetBuffer = nullptr;

	ndt::Packet PacketLayout;

	//Makeing a Handshake packet and sending it
	{
		ndt::Handshake Handshake(758, 2);
		PacketLayout.PacketID.Write(packetsSent);

		PacketLayout.dataLength = Handshake.protVer.length + Handshake.nState.length + sizeof(unsigned short) + serverAddress.length();

		packetSize = PacketLayout.CalcLength() - 1;
		packetBuffer = (char*)malloc(packetSize);
		if (packetBuffer == nullptr)
		{
			std::cout << "Not enough memory to allocate a packet" << std::endl;
			std::system("pause");
			return -1;
		}
		memset(packetBuffer, 0, packetSize);
		Handshake.DataFill(packetBuffer, PacketLayout, &serverAddress, port);

		//Sending the Packet and checking if It was sent Correctly
		byteCount = send(cSocket, packetBuffer, packetSize, 0);
		if (!(byteCount > 0))
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return 0;
		}
		//If Visual Studio is Set to Debug than show the packet and it's size in bytes
		#ifdef _DEBUG
		printf("Message Sent: %s \n", packetBuffer);
		printf("Bytes Sent: %d \n", byteCount);
		#endif

		packetsSent++;

		//If Visual Studio is Set to Debug than check for any error messages from the sever
		#ifdef _DEBUG
		char buffer2[200];
		byteCount = recv(cSocket, buffer2, 50, 0);
		if (byteCount > 0)
		{
			printf("Message Recived: %s \n", buffer2);
			printf("Bytes Recived: %d \n", byteCount);
		}
		#endif

	}

	PacketLayout.Empty();

	//Makeing a Login Start packet and sending it
	{
		bool premium = false;
		std::string nickname = "test\n";
		ndt::LoginStartP LoginP(nickname, premium);
		PacketLayout.PacketID.Write(packetsSent);

		//Smallest size that LoginStartP Packet can have (Nickname Length + 2 Bools)
		PacketLayout.dataLength += ndt::chrlen(&nickname[0]) + 2;

		//If Has UUID adds aditional 16 bytes of size
		if (LoginP.HUUID)
		{
			PacketLayout.dataLength += 16;
		}

		//If useing premium adds even more bytes
		if (premium)
		{
			PacketLayout.dataLength = 8 + LoginP.pKeyLength.length + LoginP.pKeyLength.Read() +
				LoginP.SignatureLength.length + LoginP.SignatureLength.Read();
		}

		packetSize = PacketLayout.CalcLength();
		packetBuffer = (char*)realloc(packetBuffer, packetSize);
		if (packetBuffer == nullptr)
		{
			std::cout << "Not enough memory to allocate a packet" << std::endl;
			std::system("pause");
			return -1;
		}
		memset(packetBuffer, 0, packetSize);
		LoginP.DataFill(premium, packetBuffer, PacketLayout);

		//Sending the Packet and checking if It was sent Correctly
		byteCount = send(cSocket, packetBuffer, packetSize, 0);
		if (!(byteCount > 0))
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return 0;
		}
		//If Visual Studio is Set to Debug than show the Packet and it's Size in bytes
		#ifdef _DEBUG
		printf("Message Sent: %s \n", packetBuffer);
		printf("Bytes Sent: %d \n", byteCount);
		#endif

		packetsSent++;

		//Recive Login Success Packet from the server
		bool n_Recived = true;
		while (n_Recived)
		{
			char buffer2[1024];
			byteCount = recv(cSocket, buffer2, 1024, 0);
			if (byteCount > 0)
			{
				printf("Message Recived: %s \n", buffer2);
				printf("Bytes Recived: %d \n", byteCount);

				n_Recived = false;
			}
		}
	}
	WSACleanup();
	return 0;
}