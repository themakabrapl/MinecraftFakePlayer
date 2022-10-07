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
	int serverAddressLength = 0;
	unsigned short port = 25565;
	const char* serverAddress = "127.0.0.1\n";

	while (serverAddress[serverAddressLength] != '\n')
	{
		serverAddressLength++;
	}

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

	//Setting up variables used in communication
	int byteCount = 0;
	int packetSize = 0;
	int packetsSent = 0;
	int portSize = sizeof(port);
	char* packetBuffer = nullptr;

	ndt::Packet PacketLayout;

	//Makeing a Handshake packet and sending it
	{
		ndt::Handshake Handshake;
		
		Handshake.protVer.Write(758);
		Handshake.nState.Write(2);
		PacketLayout.PacketID.Write(packetsSent);

		PacketLayout.dataLenght = Handshake.protVer.length + Handshake.nState.length + sizeof(unsigned short) + serverAddressLength;

		packetSize = PacketLayout.CalcLength() - 1;
		packetBuffer = (char*)malloc(packetSize);
		if (packetBuffer == nullptr)
		{
			std::cout << "Not enough memory to allocate a packet" << std::endl;
			std::system("pause");
			return -1;
		}
		memset(packetBuffer, 0, packetSize);
		Handshake.DataFill(packetBuffer, PacketLayout, serverAddress, serverAddressLength, port);

		byteCount = send(cSocket, packetBuffer, packetSize, 0);
		if (byteCount > 0)
		{
			std::printf("Message Sent: %s \n", packetBuffer);
			std::printf("Bytes Sent: %d \n", byteCount);
			packetsSent++;
		}
		else
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return 0;
		}
	}

	PacketLayout.Empty();

	{
		bool premium = false;
		std::string nickname = "themakabrapl2";
		ndt::LoginStartP LoginP(nickname, premium);
		PacketLayout.PacketID.Write(packetsSent);

		//Smallest size that LoginStartP Packet can have
		PacketLayout.dataLenght += 16 + 1;

		//If Has UUID adds aditional 17 bytes of size
		if (LoginP.HUUID)
		{
			PacketLayout.dataLenght += 1 + 16;
		}

		//If useing premium adds even more bytes
		if (premium)
		{
			PacketLayout.dataLenght = 8 + LoginP.pKeyLength.length + LoginP.pKeyLength.Read() +
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

		byteCount = send(cSocket, packetBuffer, packetSize, 0);
		if (byteCount > 0)
		{
			std::cout.write(packetBuffer, packetSize);
			std::printf("Message Sent: %s \n", packetBuffer);
			std::printf("Bytes Sent: %d \n", byteCount);
			packetsSent++;
		}
		else
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return 0;
		}
	}
	WSACleanup();
	return 0;
}