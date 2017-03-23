#include <fstream>
#include <iostream>

#include "TRAS_Model.h"
#include "TRDE_Model.h"

#include "Merger.h"
#include "Converter.h"

#include <cassert>

int main()
{
	std::ifstream ifs("lara.trdemesh", std::ios::binary);
	if (!ifs.good())
	{
		std::cout << "Failed to open TRDE Model file!" << std::endl;
		return 0;
	}

	std::ofstream ofs("test.bin", std::ios::binary);
	if (!ofs.good())
	{
		std::cout << "Failed to open TRAS Model file for writing!" << std::endl;
		return 0;
	}

	Converter::Convert(ifs, ofs);

	ifs.close();
	ofs.close();

	//We can take advantage of the vertBuffer pointers. remap everything to EOF?
	return 0;
}