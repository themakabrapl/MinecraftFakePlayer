#pragma once

#include <vector>
#include <bitset>

#include "Functions.h"

/* This File contains required Datatypes */

/* More Information about Minecraft Packets can be Found Here:
	https://wiki.vg/Protocol#Login_Start */

//Namespace containing new required datatypes and functions
namespace rdt
{
	//New Datatypes
	class VarInt
	{
	public:
		char* data = nullptr;
		bool freeData = false;
		unsigned long length = 0;

		int Read();
		void Write(size_t value);
		void Assign(char* ptr);

		~VarInt();
	private:
		int SEGMENT_BIT_MASK = 0x7F;
		int CONTINUE_BIT_MASK = 0x80;
	};

	//Reads the Content of a VarInt
	int VarInt::Read()
	{
		int value = 0;
		int position = 0;
		byte currentByte;

		int i = 0;
		while (true)
		{
			currentByte = data[i];
			value |= (currentByte & SEGMENT_BIT_MASK) << position;

			i++;
			position += 7;

			if ((currentByte & CONTINUE_BIT_MASK) == 0)
				break;

			if (position >= 32)
				std::cout << "VarInt is too Big" << std::endl;
		}
		length = position / 7;
		return value;
	};

	//Writes to a VarInt
	void VarInt::Write(size_t value)
	{
		unsigned int cValue = (unsigned int)value;
		bool state = true;
		std::vector<byte> bytes;

		if (data != nullptr)
			free(data);

		while (state)
		{
			if ((cValue & ~SEGMENT_BIT_MASK) == 0)
			{
				bytes.push_back(cValue);
				state = false;
				break;
			}
			bytes.push_back(((cValue & SEGMENT_BIT_MASK) | CONTINUE_BIT_MASK));
			cValue >>= 7;
		}

		length = bytes.size();

		data = (char*)malloc(length);
		if (data == nullptr)
		{
			std::cout << "Not enough memory to Write to a VarInt" << std::endl;
		}

		int i = 0;
		while (i != length)
		{
			data[i] = bytes.at(i);
			i++;
		}
	}

	void VarInt::Assign(char* ptr)
	{
		data = ptr;
		freeData = false;
		Read();
	};

	VarInt::~VarInt()
	{
		if (freeData == true)
			free(data);
	};


	//A Packet Class
	class Packet
	{
	public:
		rdt::VarInt Length;
		rdt::VarInt DataLength;
		rdt::VarInt PacketID;

		unsigned int dataLength = 0;
		unsigned int packetDataOffset = 0;
		char* packetBuffer = nullptr;
		char* data = nullptr;	//Pointer to the data part of a packet

		void Empty();
		unsigned int CalcLength();
	};

	//Resets the variables that need to be reseted (QOL)
	void Packet::Empty()
	{
		dataLength = 0;
		packetDataOffset = 0;
	}

	//Calculates the Length of a packet in Bytes
	unsigned int Packet::CalcLength()
	{
		unsigned int value = 0;
		Length.Write(PacketID.length + dataLength);

		value += Length.length;
		value += PacketID.length;
		value += dataLength;

		return value;
	}

	//A Handshake Packet
	struct Handshake
	{
		unsigned int PacketID = 0;
		rdt::VarInt protVer;
		rdt::VarInt nState;

		void DataFill(char* packetBuffer, Packet &PacketLayout,std::string* serverAddres, short port);

		Handshake(int protocolNumber, int nextState);
	};

	//Fills a Buffer with Variables in a specific for Handshake Packet Layout
	void Handshake::DataFill(char* packetBuffer, Packet& PacketLayout, std::string* serverAddress, short port)
	{
		unsigned int serverAddressLength = serverAddress->length();
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.Length.data[0], PacketLayout.Length.length);
		PacketLayout.packetDataOffset += PacketLayout.Length.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.PacketID.data[0], PacketLayout.PacketID.length);
		PacketLayout.packetDataOffset += PacketLayout.PacketID.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &protVer.data[0], protVer.length);
		PacketLayout.packetDataOffset += protVer.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &(*serverAddress)[0], serverAddressLength);
		PacketLayout.packetDataOffset += serverAddressLength;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], ((char*)(&port)) + 1, 1);
		PacketLayout.packetDataOffset += 1;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &port, 1);
		PacketLayout.packetDataOffset += 1;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &nState.data[0], nState.length);
		PacketLayout.packetDataOffset += nState.length;
	}

	//Constructor of Handshake Packet (Takes in Protocol Number and Next State Value)
	Handshake::Handshake(int protocolNumber, int nextState)
	{
		protVer.Write(protocolNumber);
		nState.Write(nextState);
	}

	//A LoginStart Packet
	struct LoginStartP
	{
		int PacketID = 0;
		char Name[17];
		bool SigData = false;

		long TimeStamp;
		VarInt pKeyLength;
		char* PublicKey = nullptr;
		VarInt SignatureLength;
		char* Signature = nullptr;
		bool HUUID = false;
		UUID UUID;
		//UuidCreate(&LoginP.UUID)

		void DataFill(bool premium, char* packetBuffer, Packet& PacketLayout);

		LoginStartP(std::string nickname, bool Premium);
		~LoginStartP();
	};

	//Fills a Buffer with Variables in a specific for Login Start Packet Layout
	void LoginStartP::DataFill(bool premium, char* packetBuffer, Packet &PacketLayout)
	{
		byte nameLength = chrlen(Name);
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.Length.data[0], PacketLayout.Length.length);
		PacketLayout.packetDataOffset += PacketLayout.Length.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.PacketID.data[0], PacketLayout.PacketID.length);
		PacketLayout.packetDataOffset += PacketLayout.PacketID.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &nameLength, 1);
		PacketLayout.packetDataOffset += 1;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &Name[0], chrlen(Name));
		PacketLayout.packetDataOffset += chrlen(Name);
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &SigData, 1);
		PacketLayout.packetDataOffset += 1;

		if (premium)
		{
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &TimeStamp, 8);
			PacketLayout.packetDataOffset += 8;
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &pKeyLength.data[0], pKeyLength.length);
			PacketLayout.packetDataOffset += pKeyLength.length;
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PublicKey[0], pKeyLength.Read());
			PacketLayout.packetDataOffset += pKeyLength.Read();
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &SignatureLength.data[0], SignatureLength.length);
			PacketLayout.packetDataOffset += SignatureLength.length;
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &Signature[0], SignatureLength.Read());
			PacketLayout.packetDataOffset += SignatureLength.Read();
		}

		if (HUUID)
		{
			RPC_WSTR str;
			UuidToString(&UUID, &str);
			std::cout << "UUID: " << str << std::endl;

			memset(&packetBuffer[PacketLayout.packetDataOffset], HUUID, 1);
			PacketLayout.packetDataOffset += 1;
			memcpy(&packetBuffer[PacketLayout.packetDataOffset], &str[0], 16);
			PacketLayout.packetDataOffset += 16;
		}
	}

	//Constructor of Login Start Packet (Takes in Nickname of a Player and is It a Premium Account)
	LoginStartP::LoginStartP(std::string nickname, bool Premium)
	{
		memset(Name, 0, 16);
		memcpy(Name, &nickname[0], nickname.length());
		memset(Name + nickname.length(), 0, 16 - nickname.length());
		SigData = Premium;
	}

	LoginStartP::~LoginStartP()
	{
		if (PublicKey != nullptr)
			free(PublicKey);

		if (Signature != nullptr)
			free(Signature);
	}

	class Property
	{
	public:
		bool isSigned;
		char* name = nullptr;
		char* value = nullptr;
		char* signature = nullptr;

		Property(char* n);
		~Property();
	};

	Property::Property(char* startingAddress)
	{
		name = (char*)malloc(32767);
		value = (char*)malloc(32767);

		memcpy(name, startingAddress, 32767);
		memcpy(value, startingAddress + 32767, 32767);
		memcpy(&isSigned, startingAddress + 65534, 1);

		if (isSigned == true)
		{
			signature = (char*)malloc(32767);
			memcpy(signature, startingAddress + 65534 + 1, 32767);
		}
	}

	Property::~Property()
	{
		free(name);
		free(value);

		if (isSigned == true)
			free(signature);
	}

	struct SessionInformation
	{
		char username[16];
		UUID uuid;
	};
}