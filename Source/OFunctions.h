#pragma once

#include "Datatypes.h"

int decompressPacket(rdt::Packet& rPacket, unsigned long& sourceLength, unsigned long& uncompressedDataSize, char* uncompressedDataAddress)
{
	sourceLength = rPacket.Length.Read() - rPacket.DataLength.length;
	uncompressedDataSize = rPacket.DataLength.Read();

	if (uncompressedDataSize == 0)
	{
		uncompressedDataAddress = rPacket.data + 1;
		rPacket.PacketID.Assign(uncompressedDataAddress);
		return 0;
	}

	uncompressedDataAddress = (char*)malloc(uncompressedDataSize);
	if (uncompressedDataAddress == nullptr)
	{
		std::cout << "Not enough memory to be decompress a Packet" << std::endl;
		std::system("pause");
		return EXIT_FAILURE;
	}

	int decompression = uncompress2((Bytef*)uncompressedDataAddress, (uLongf*)&uncompressedDataSize,
		(Bytef*)(rPacket.DataLength.data + rPacket.DataLength.length), (uLong*)&sourceLength);

	if (decompression != Z_OK)
	{
		if (decompression == Z_MEM_ERROR)
			std::cout << "Couldn't decompress the Packet because there was not enought Memory" << std::endl;

		if (decompression == Z_BUF_ERROR)
			std::cout << "Couldn't decompress the Packet because there was not enought memory in the Buffer" << std::endl;

		if (decompression == Z_DATA_ERROR)
			std::cout << "Data of the Packet was corrupted or incomplete" << std::endl;

		std::system("pause");
		return decompression;
	}
	rPacket.PacketID.Assign(uncompressedDataAddress);
	return 0;
}

bool respondToPacket(rdt::Packet& rPacket, rdt::SessionInformation& SI, int& SIZE_BEFORE_COMPESSION)
{
	int packetID = rPacket.PacketID.Read();
	if (packetID == 2)
	{
		char* username;

		std::vector<rdt::Property*> Properties;
		rdt::VarInt numberOfProperties;

		if (UuidFromString((RPC_WSTR)rPacket.data, &SI.uuid) == RPC_S_INVALID_STRING_UUID)
		{
			std::cout << "The string UUID was invalid." << std::endl;
			return -1;
		}

		username = (rPacket.data + 16);
		numberOfProperties.Assign(username + 16);

		int i = 0;
		int nop = numberOfProperties.Read();
		char* variableAddress = (numberOfProperties.data + numberOfProperties.length);
		while (i < nop)
		{
			rdt::Property* Property = new rdt::Property(variableAddress);
			Properties.push_back(Property);
			variableAddress = Property->signature + 32767;
		}

		memcpy(SI.username, username, 16);
	}

	if (packetID == 3)
	{
		rdt::VarInt tmp;
		tmp.Assign(rPacket.data);
		SIZE_BEFORE_COMPESSION = tmp.Read();
		return true;
	}

	if (packetID == 69)
	{

	}
}