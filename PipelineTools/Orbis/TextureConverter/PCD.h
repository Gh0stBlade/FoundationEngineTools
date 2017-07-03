#pragma once

#include "DDS.h"

#include <iostream>
#include <fstream>
#include <cassert>

namespace PCD
{
#pragma pack(push, 1)
	struct Header
	{
		unsigned int m_magic;
		unsigned int m_format;
		unsigned int m_pixelDataSize;
		unsigned int m_unk00;
		unsigned short m_width;
		unsigned short m_height;
		unsigned short m_bpp;
		unsigned short m_numMipMaps;
		unsigned int m_unk01;
	};
#pragma pack(pop)

	enum Format
	{
		DXT1 = 0x31545844,//FIXME i swear this needs reversing
		DXT3 = 0x33545844,
		DXT5 = 0x35545844,
	};

	unsigned int getFormat(const unsigned int& ddsFormat)
	{
		switch (ddsFormat)
		{
		case DDS::DXT1:
			return PCD::DXT1;
			break;
		case DDS::DXT3:
			return PCD::DXT3;
			break;
		case DDS::DXT5:
			return PCD::DXT5;
			break;
		default:
			assert(false);
			break;
		}

		return 0;
	}

	void Write(const DDS::File& ddsFile)
	{
		std::ofstream ofs("test.bin", std::ios::binary);
		assert(ofs.good());

		//Copy DDS header info into PCD9 header info
		PCD::Header pcdHeader;

		pcdHeader.m_magic = 0x39444350;
		pcdHeader.m_format = getFormat(ddsFile.m_header.m_format);
		pcdHeader.m_pixelDataSize = ddsFile.m_pixelDataSize;
		pcdHeader.m_unk00 = 0;
		pcdHeader.m_width = ddsFile.m_header.m_width;
		pcdHeader.m_height = ddsFile.m_header.m_height;
		pcdHeader.m_bpp = ddsFile.m_header.m_depth;
		pcdHeader.m_numMipMaps = ddsFile.m_header.m_mipCount;
		pcdHeader.m_unk01 = 0;
		ofs.write(reinterpret_cast<char*>(&pcdHeader), sizeof(PCD::Header));
		ofs.write(ddsFile.m_pixelData, ddsFile.m_pixelDataSize);
		ofs.close();
	}
};