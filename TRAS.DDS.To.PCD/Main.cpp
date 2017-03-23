#include <iostream>
#include <fstream>
#include <cassert>

#include "DDS.h"
#include "PCD.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "DDS2PCD" << std::endl << "Usage: dds_file\n" << std::endl;
	}
	else
	{
		DDS::File ddsFile;
		DDS::Read(ddsFile, argv[1]);
		PCD::Write(ddsFile);// , argv[1]);
	}
	return 0;
}

