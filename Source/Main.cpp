#include <iostream>
#include <string>
#include <vector>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <tchar.h>

#include <rpc.h>

#include "zconf.h"
#include "zlib.h"

#include "Functions.h"
#include "OFunctions.h"
#include "Datatypes.h"

int main(int argc, char** argv)
{
	unsigned short port = 25565;
	std::string serverAddress = " 127.0.0.1";
	serverAddress[0] = (char)9;

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
	int wsaerr = WSAStartup(wVersionRequested, &wsaData);

	//Checking If Initialization was done Correctly
	if (wsaerr != 0)
	{
		std::cout << "The Winsock dll not found" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << "The Winsock dll found" << std::endl;
	std::cout << "The Status:" << wsaData.szSystemStatus << std::endl;

	//Setting up client Socket
	cSocket = INVALID_SOCKET;
	cSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Checking If Socket was made Correctly
	if (cSocket == INVALID_SOCKET)
	{
		std::cout << "Error at Socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return EXIT_FAILURE;
	}
	std::cout << "Socket is OK" << std::endl;

	//Createing a Data Structure for information about ip address and port to connect to
	sockaddr_in addrpo;
	addrpo.sin_family = AF_INET;
	InetPton(AF_INET, _T("127.0.0.1"), &addrpo.sin_addr.s_addr);
	addrpo.sin_port = htons(port);

	//Connecting to Server and Checking If Connection was Successful
	if (connect(cSocket, (SOCKADDR*)&addrpo, sizeof(addrpo)) == SOCKET_ERROR)
	{
		std::cout << "Connect() failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return EXIT_FAILURE;
	}
	std::cout << "Connect() is OK" << std::endl;

	/* End of Nicholas's Code*/

	//Setting up variables used in communication
	unsigned int byteCount = 0;
	unsigned int packetSize = 0;

	rdt::Packet PacketLayout;

	//Makeing a Handshake packet and sending it
	{
		rdt::Handshake Handshake(758, 2);
		PacketLayout.PacketID.Write(Handshake.PacketID);

		PacketLayout.dataLength = Handshake.protVer.length + Handshake.nState.length + sizeof(unsigned short) + serverAddress.length();

		//Calculateing the size of a Packet and useing it to create a Buffer for It than Populateing It
		packetSize = PacketLayout.CalcLength();
		PacketLayout.packetBuffer = (char*)malloc(packetSize);
		if (PacketLayout.packetBuffer == nullptr)
		{
			std::cout << "Not enough memory to allocate a packet" << std::endl;
			std::system("pause");
			return EXIT_FAILURE;
		}
		memset(PacketLayout.packetBuffer, 0, packetSize);
		Handshake.DataFill(PacketLayout.packetBuffer, PacketLayout, &serverAddress, port);

		//Sending the Packet and checking if It was sent Correctly
		byteCount = send(cSocket, PacketLayout.packetBuffer, packetSize, 0);
		if (!(byteCount > 0))
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return EXIT_FAILURE;
		}
		//If Visual Studio is Set to Debug than show the packet and it's size in bytes
		#ifdef _DEBUG
		printf("Message Sent: %s \n", PacketLayout.packetBuffer);
		printf("Bytes Sent: %d \n", byteCount);
		#endif
	}

	PacketLayout.Empty();

	//Makeing a Login Start packet and sending it
	{
		bool premium = false;
		std::string nickname = "themakabrapl2\n";
		rdt::LoginStartP LoginP(nickname, premium);
		PacketLayout.PacketID.Write(LoginP.PacketID);

		//Smallest size that LoginStartP Packet can have (Nickname Length + 1 Byte)
		PacketLayout.dataLength += chrlen(&nickname[0]) + 1;

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

		//Calculateing the size of a Packet and useing it to create a Buffer for It than Populateing It
		packetSize = PacketLayout.CalcLength();
		PacketLayout.packetBuffer = (char*)realloc(PacketLayout.packetBuffer, packetSize);
		if (PacketLayout.packetBuffer == nullptr)
		{
			std::cout << "Not enough memory to allocate a packet" << std::endl;
			std::system("pause");
			return EXIT_FAILURE;
		}
		memset(PacketLayout.packetBuffer, 0, packetSize);
		LoginP.DataFill(premium, PacketLayout.packetBuffer, PacketLayout);

		//Sending the Packet and checking if It was sent Correctly
		byteCount = send(cSocket, PacketLayout.packetBuffer, packetSize, 0);
		if (!(byteCount > 0))
		{
			std::cout << "Sending Packet was a failure" << std::endl;
			std::system("pause");
			WSACleanup();
			return EXIT_FAILURE;
		}
		//If Visual Studio is Set to Debug than show the Packet and it's Size in bytes
		#ifdef _DEBUG
		printf("Message Sent: %s \n", PacketLayout.packetBuffer);
		printf("Bytes Sent: %d \n", byteCount);
		#endif
	}

	{
		//Variables for Reciveing Packets
		unsigned int pID = NULL;

		unsigned long sourceLength = 0;
		unsigned long uncompressedDataSize = 0;

		char* reciveBuffer = nullptr;
		char* uncompressedDataAddress = nullptr;

		rdt::VarInt uVInt;

		reciveBuffer = (char*)malloc(2097152);
		if (reciveBuffer == nullptr)
		{
			std::cout << "Not enough memory to be reciveing a Packets" << std::endl;
			std::system("pause");
			return EXIT_FAILURE;
		}
		memset(reciveBuffer, 0, 2097152);

		//Flags used in Communication
		int SIZE_BEFORE_COMPESSION = -1;

		rdt::SessionInformation SI;

		//Loop for Reciveing Packets and responding to Them
		while (true)
		{
			rdt::Packet rPacket;

			//Recive a Packet/Maybe Packets idk. and check for Errors 
			byteCount = recv(cSocket, reciveBuffer, 2097152, 0);
			if (byteCount == SOCKET_ERROR)
			{
				std::cout << "Recv() Failed: " << WSAGetLastError() << std::endl;
				continue;
			}

			//Get Length of the received Packet and If it's 0 go back and recieve another Packet
			rPacket.Length.Assign(reciveBuffer);
			if (rPacket.Length.length <= 0)
			{
				continue;
			}

			//If Flag SIZE_BEFORE_COMPESSION > 0 decompress the packet
			[[unlikely]] if (SIZE_BEFORE_COMPESSION < 0)
			{
				rPacket.PacketID.Assign(reciveBuffer + rPacket.Length.length);
				rPacket.data = rPacket.PacketID.data + rPacket.PacketID.length;
				respondToPacket(rPacket, SI, SIZE_BEFORE_COMPESSION);
			}

			rPacket.DataLength.Assign(reciveBuffer + rPacket.Length.length);
			rPacket.data = rPacket.DataLength.data + rPacket.DataLength.length;

			int res = decompressPacket(rPacket, sourceLength, uncompressedDataSize, uncompressedDataAddress);
			if (res != 0)
			{
				std::cout << "Decompression of a packet failed: " << res << std::endl;
				return EXIT_FAILURE;
			}

			respondToPacket(rPacket, SI, SIZE_BEFORE_COMPESSION);
			memset(reciveBuffer, 0, byteCount);
		}
	}
	WSACleanup();
	return 0;
}