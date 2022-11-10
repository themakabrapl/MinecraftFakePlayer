#pragma once

#include <vector>
#include <bitset>

/* More Information about Minecraft Packets can be Found Here:
	https://wiki.vg/Protocol#Login_Start */

//Namespace containing new required datatypes and functions
namespace ndt
{
	//New Functions
	int chrlen(const char* array)
	{
		int i = 0;
		while (array[i] != '\n')
		{
			i++;
		}
		return i;
	}

	//New Datatypes
	class VarInt
	{
	public:
		char* data = nullptr;
		int length = 0;

		int Read();
		void Write(int value);

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

			if ((currentByte & CONTINUE_BIT_MASK) == 0)
				break;

			i++;
			position += 7;

			if (position >= 32)
				std::cout << "VarInt is too Big" << std::endl;
		}
		return value;
	};

	//Writes to a VarInt
	void VarInt::Write(int value)
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
	};

	VarInt::~VarInt()
	{
		free(data);
	};


	//Defines layout for a Packet
	class Packet
	{
	public:
		ndt::VarInt Length;
		ndt::VarInt PacketID;

		int dataLength = 0;
		int packetDataOffset = 0;

		void Empty();
		int CalcLength();
	};

	//Resets the variables that need to be reseted (QOL)
	void Packet::Empty()
	{
		dataLength = 0;
		packetDataOffset = 0;
	}

	//Calculates the Length of a packet in Bytes
	int Packet::CalcLength()
	{
		int value = 0;
		Length.Write(PacketID.length + dataLength);

		value += Length.length;
		value += PacketID.length;
		value += dataLength;

		return value;
	}
	//A Handshake Packet
	struct Handshake
	{
		ndt::VarInt protVer;
		ndt::VarInt nState;

		void DataFill(char* packetBuffer, Packet &PacketLayout,std::string* serverAddres, short port);

		Handshake(int protocolNumber, int nextState);
	};

	//Fills a Buffer with Variables in a specific for Handshake Packet Layout
	void Handshake::DataFill(char* packetBuffer, Packet& PacketLayout, std::string* serverAddress, short port)
	{
		int serverAddressLength = serverAddress->length();
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.Length.data[0], PacketLayout.Length.length);
		PacketLayout.packetDataOffset += PacketLayout.Length.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.PacketID.data[0], PacketLayout.PacketID.length);
		PacketLayout.packetDataOffset += PacketLayout.PacketID.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &protVer.data[0], protVer.length);
		PacketLayout.packetDataOffset += protVer.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &serverAddress[0], serverAddressLength);
		PacketLayout.packetDataOffset += serverAddressLength;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &port, 2);
		PacketLayout.packetDataOffset += sizeof(port);
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
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.Length.data[0], PacketLayout.Length.length);
		PacketLayout.packetDataOffset += PacketLayout.Length.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &PacketLayout.PacketID.data[0], PacketLayout.PacketID.length);
		PacketLayout.packetDataOffset += PacketLayout.PacketID.length;
		memcpy(&packetBuffer[PacketLayout.packetDataOffset], &Name[0], ndt::chrlen(Name));
		PacketLayout.packetDataOffset += ndt::chrlen(Name);
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
}