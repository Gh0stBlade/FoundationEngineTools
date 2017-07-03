#pragma once

#include <iostream>
#include <fstream>
#include <cassert>

namespace DDS
{
#pragma pack(push, 1)
	struct Header
	{
		unsigned int m_magic;
		unsigned int m_size;
		unsigned int m_flags;
		unsigned int m_height;

		unsigned int m_width;
		unsigned int m_pitchOrLinearSize;
		unsigned int m_dummy;
		unsigned int m_depth;
		unsigned int m_mipCount;

		unsigned int m_reserved[12];
		unsigned int m_format;
		unsigned int m_reserved2[10];
	};

	struct File
	{
		DDS::Header m_header;
		unsigned int m_pixelDataSize;
		char*		m_pixelData;
	};

#pragma pack(pop)

	enum Format
	{
		DXT1 = 0x31545844,
		DXT3 = 0x33545844,
		DXT5 = 0x35545844
	};

	void Read(DDS::File& ddsFile, const char* filePath)
	{
		std::ifstream ifs(filePath, std::ios::binary);
		ifs.seekg(0, SEEK_END);
		unsigned int fileLength = static_cast<unsigned int>(ifs.tellg());
		ifs.seekg(0, SEEK_SET);
		assert(ifs.good());
		ddsFile.m_pixelDataSize = fileLength - sizeof(DDS::Header);
		ifs.read(reinterpret_cast<char*>(&ddsFile.m_header), sizeof(DDS::Header));
		ddsFile.m_pixelData = new char[fileLength - sizeof(DDS::Header)];
		ifs.read(ddsFile.m_pixelData, fileLength - sizeof(DDS::Header));
		ifs.close();
	}

	void Destroy(DDS::File& ddsFile)
	{
		delete[] ddsFile.m_pixelData;
	}
};