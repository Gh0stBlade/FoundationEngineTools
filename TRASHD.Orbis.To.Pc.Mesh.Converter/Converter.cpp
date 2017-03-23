#include "Converter.h"
#include "TRDE_Model.h"
#include "TRAS_Model.h"

#include <cassert>
#include <iostream>
#include <climits>

const unsigned int MAX_BONES_PER_MESH_GROUP = 42;
//TODO bi normal/tangent calculations
#define REMAP_TO_PC_SKELETON (1)

//TODO: Optimise vertex buffer.
//TODO: Pull relocation data into TRDE result file, move bones accordingly
void Converter::Convert(std::ifstream& inStream, std::ofstream& outStream)
{
	TRDE::Model model;
	TRDE::loadModel(model, inStream);
	
	std::vector<TRDE::MeshGroup> newMeshGroups;
	std::vector<TRDE::FaceGroup> newFaceGroups;
	std::vector<std::vector<int>> boneRemapTables;
	std::vector<unsigned short*> newIndexBuffers;
	std::vector<char*> newVertexBuffers;
	std::vector<TRDE::VertexDeclarationHeader> newVertexDeclarationHeaders;
	std::vector<std::vector<TRDE::VertexDeclaration>> newVertexDeclarations;

	//Process all mesh groups, PC engine has a limit of 42 bones per mesh group so here we split the mesh/face groups accordingly if necessary.
	unsigned int currentFaceGroup = 0;
	for (unsigned int i = 0; i < model.m_meshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = model.m_meshGroups[i];

		//Get the skin indices vertex decl info
		TRDE::VertexDeclarationHeader& vertDeclHeader = model.m_vertexDeclarationHeaders[i];
		std::vector<TRDE::VertexDeclaration>& vertDecls = model.m_vertexDeclarations[i];
		TRDE::VertexDeclaration* skinIndexDecl = nullptr;

		for (unsigned int j = 0; j < vertDeclHeader.m_componentCount; j++)
		{
			TRDE::VertexDeclaration& vertDecl = vertDecls[j];
			if (vertDecl.m_componentNameHashed == 0x5156D8D3)
			{
				skinIndexDecl = &vertDecl;
				break;
			}
		}

		if (skinIndexDecl)//Todo mesh header flag
		{
			//For each face group in this mesh group, let's generate the bone remap table.
			for (unsigned int j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
			{
				TRDE::FaceGroup& faceGroup = model.m_faceGroups[currentFaceGroup];
				Converter::GenerateBoneRemapTable(currentMeshGroup, faceGroup, vertDeclHeader, model.m_vertexBuffers[i], reinterpret_cast<unsigned short*>(model.m_indexBuffers[currentFaceGroup]), skinIndexDecl, boneRemapTables, newMeshGroups, newFaceGroups, newIndexBuffers, newVertexBuffers, newVertexDeclarationHeaders, vertDecls, newVertexDeclarations);
			}
		}
		else
		{
			newVertexDeclarationHeaders.push_back(vertDeclHeader);
			newVertexDeclarations.push_back(vertDecls);
			newMeshGroups.push_back(currentMeshGroup);

			newVertexBuffers.push_back(model.m_vertexBuffers[i]);
			for (unsigned int j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
			{
				TRDE::FaceGroup& faceGroup = model.m_faceGroups[currentFaceGroup];
				newFaceGroups.push_back(faceGroup);
				newIndexBuffers.push_back((unsigned short*)model.m_indexBuffers[currentFaceGroup]);
			}
		}
	}

	model.m_meshHeader.m_fileVersion = 0x0E01;
	outStream.write(reinterpret_cast<char*>(&model.m_meshHeader), 112);
	outStream.seekp(48, SEEK_CUR);

	unsigned int offsetBoneIndexList = static_cast<unsigned int>(outStream.tellp());

	//Load TRAS/TRDE skeleton
	TRDE::Skeleton skeleton;
	if (model.m_meshHeader.m_primitiveDrawType > 0)
	{
		TRDE::loadSkeleton(skeleton);
	}

	TRAS::Skeleton trasSkel;
	if (model.m_meshHeader.m_primitiveDrawType > 0)
	{
		TRAS::loadSkeleton(trasSkel);
	}

	//Write bone index list
	for (size_t i = 0; i < trasSkel.m_bones.size(); i++)
	{
		outStream.write(reinterpret_cast<char*>(&i), sizeof(unsigned int));
	}
	outStream.seekp(((static_cast<unsigned int>(outStream.tellp()) + 15) & ~15), SEEK_SET);

	unsigned int offsetMeshGroups = static_cast<unsigned int>(outStream.tellp());
	unsigned int boneRemapTableStart = offsetMeshGroups + newMeshGroups.size() * sizeof(TRAS::MeshGroup);

	for (size_t i = 0; i < newMeshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = newMeshGroups[i];
		unsigned short test = boneRemapTables.size() > 0 ? boneRemapTables[i].size() : 0;
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_numFaceGroups), sizeof(unsigned int));
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_lodIndex), sizeof(unsigned short));
		outStream.write(reinterpret_cast<char*>(&test), sizeof(unsigned short));
		outStream.write(reinterpret_cast<char*>(&boneRemapTableStart), sizeof(unsigned int));
		boneRemapTableStart += test * sizeof(unsigned int);
		boneRemapTableStart = (((boneRemapTableStart + 15) & ~15));
		//Need to do offsetVertexBuff, offsetFVF and vertCount
		//assert(false);
		outStream.seekp(36, SEEK_CUR);
	}

	//Generate TRDE Skeleton to TRAS Skeleton bone remap table.
	std::vector<int> skeletonRemapTable;
	GenerateGlobalBoneRemapTable(skeletonRemapTable, trasSkel.m_bones, skeleton.m_bones);

	for (size_t i = 0; i < boneRemapTables.size(); i++)
	{
		for (size_t j = 0; j < boneRemapTables[i].size(); j++)
		{
			boneRemapTables[i][j] = skeletonRemapTable[boneRemapTables[i][j]];
#if _DEBUG
			assert(boneRemapTables[i][j] < trasSkel.m_bones.size());
#endif
		}
	}

	//Writing the bone remap tables.
	for (size_t i = 0; i < boneRemapTables.size(); i++)
	{
		for (size_t j = 0; j < boneRemapTables[i].size(); j++)
		{
			outStream.write(reinterpret_cast<char*>(&boneRemapTables[i][j]), sizeof(int));
		}
		outStream.seekp(((static_cast<unsigned int>(outStream.tellp()) + 15) & ~15), SEEK_SET);
	}

	unsigned int currentVertexDeclaration = 0;
	for (size_t i = 0; i < newMeshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = newMeshGroups[i];
		TRDE::VertexDeclarationHeader& vertDeclHeader = newVertexDeclarationHeaders[i];

		currentMeshGroup.m_offsetFVFInfo = static_cast<unsigned int>(outStream.tellp());
		outStream.write(reinterpret_cast<char*>(&newVertexDeclarationHeaders[i]), sizeof(TRDE::VertexDeclarationHeader));

		for (size_t j = 0; j < newVertexDeclarations[i].size(); j++)
		{
			outStream.write(reinterpret_cast<char*>(&newVertexDeclarations[i][j]), sizeof(TRDE::VertexDeclaration));
			
			switch (newVertexDeclarations[i][j].m_componentNameHashed)
			{
			case 0x36F5E414://Normal
				//Convert normals
				for (unsigned int k = 0; k < currentMeshGroup.m_numVertices; k++)
				{
					char* normalPointer = newVertexBuffers[i] + (k*vertDeclHeader.m_vertexStride) + newVertexDeclarations[i][j].m_position;
					decodeNormal(normalPointer);
				}
				break;
			case 0xF1ED11C3://Tangent
				//Convert tangents
				for (unsigned int k = 0; k < currentMeshGroup.m_numVertices; k++)
				{
					char* tangentPointer = newVertexBuffers[i] + (k*vertDeclHeader.m_vertexStride) + newVertexDeclarations[i][j].m_position;
					//decodeNormal(tangentPointer);
				}
				break;
			case 0x64A86F01://Bi Normal
				//Bi normals
				for (unsigned int k = 0; k < currentMeshGroup.m_numVertices; k++)
				{
					char* biNormalPointer = newVertexBuffers[i] + (k*vertDeclHeader.m_vertexStride) + newVertexDeclarations[i][j].m_position;
					//decodeNormal(biNormalPointer);
				}
				break;
			default:
				break;
			}
		}
		
		char* vertexBuffer = newVertexBuffers[i];
		currentMeshGroup.m_offsetVertexBuffer = static_cast<unsigned int>(outStream.tellp());

		outStream.write(vertexBuffer, vertDeclHeader.m_vertexStride*currentMeshGroup.m_numVertices);
		outStream.seekp(((static_cast<unsigned int>(outStream.tellp()) + 15) & ~15), SEEK_SET);
		outStream.seekp(16, SEEK_CUR);
	}

	unsigned int offsetIndexBuffer = 0;
	unsigned int offsetFaceGroups = 0;
	std::vector<unsigned int> indexBufferOffsets;
	offsetIndexBuffer = static_cast<unsigned int>(outStream.tellp());
	
	//Index buffers
	currentFaceGroup = 0;
	for (size_t i = 0; i < newMeshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = newMeshGroups[i];
		
		for (size_t j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			TRDE::FaceGroup& faceGroup = newFaceGroups[currentFaceGroup];

			indexBufferOffsets.push_back(static_cast<unsigned int>(outStream.tellp()) - offsetIndexBuffer);
			char* faceBuff = reinterpret_cast<char*>(newIndexBuffers[currentFaceGroup]);
			if (model.m_meshHeader.m_primitiveDrawType > 0)//TODO: Switch
			{
				outStream.write(faceBuff, (faceGroup.m_numFaces * 3) * sizeof(unsigned short));
			}
			else
			{
				outStream.write(faceBuff, (faceGroup.m_numFaces * 2) * sizeof(unsigned short));
			}
		}
	}

	//Face groups
	outStream.seekp(((static_cast<unsigned int>(outStream.tellp()) + 15) & ~15), SEEK_SET);
	outStream.seekp(16, SEEK_CUR);//?
	offsetFaceGroups = static_cast<unsigned int>(outStream.tellp());
	currentFaceGroup = 0;
	for (size_t i = 0; i < newMeshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = newMeshGroups[i];
		
		for (size_t j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			newFaceGroups[currentFaceGroup].m_offsetFaceStart = indexBufferOffsets[currentFaceGroup]/2;
			outStream.write(reinterpret_cast<char*>(&newFaceGroups[currentFaceGroup]), 32);
			outStream.seekp(8, SEEK_CUR);
			int temp = 0;// newFaceGroups[currentFaceGroup].m_unk04;
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			outStream.seekp(12, SEEK_CUR);
			//temp = newFaceGroups[currentFaceGroup].m_unk06;
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			temp = -1;
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
			outStream.write(reinterpret_cast<char*>(&temp), sizeof(int));
		}
	}

	outStream.seekp(112, SEEK_SET);
	outStream.seekp(4, SEEK_CUR);//TODO: merge seek
	outStream.write(reinterpret_cast<char*>(&offsetFaceGroups), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetMeshGroups), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetBoneIndexList), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetBoneIndexList), sizeof(unsigned int));//Lodinfo!
	outStream.write(reinterpret_cast<char*>(&offsetIndexBuffer), sizeof(unsigned int));
	unsigned short numFaceGroups = newFaceGroups.size();
	outStream.write(reinterpret_cast<char*>(&numFaceGroups), sizeof(unsigned short));
	unsigned short numMeshGroups = newMeshGroups.size();
	outStream.write(reinterpret_cast<char*>(&numMeshGroups), sizeof(unsigned short));

	unsigned short numBones = trasSkel.m_bones.size();
	outStream.write(reinterpret_cast<char*>(&numBones), sizeof(unsigned short));
	outStream.seekp(4, SEEK_CUR);//Num lods

	//Write updated mesh groups
#if 0
	outStream.seekp(offsetMeshGroups, SEEK_SET);
	for (size_t i = 0; i < newMeshGroups.size(); i++)
	{
		TRDE::MeshGroup& currentMeshGroup = newMeshGroups[i];
		unsigned int test = boneRemapTables.size() > 0 ? boneRemapTables[i].size() : 0;
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_numFaceGroups), sizeof(unsigned int));
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_lodIndex), sizeof(unsigned short));
		outStream.write(reinterpret_cast<char*>(&test), sizeof(unsigned short));
		if (boneMapOffsets.size() > 0)
		{
			outStream.write(reinterpret_cast<char*>(&boneMapOffsets[i]), sizeof(unsigned int));//Offset bone map uint
		}
		else
		{
			outStream.seekp(4, SEEK_CUR);//Padding
		}
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_offsetVertexBuffer), sizeof(unsigned int));
		outStream.seekp(12, SEEK_CUR);//Padding
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_offsetFVFInfo), sizeof(unsigned int));
		outStream.write(reinterpret_cast<char*>(&currentMeshGroup.m_numVertices), sizeof(unsigned int));
		outStream.seekp(4, SEEK_CUR);//Padding
		outStream.seekp(8, SEEK_CUR);
	}
#endif

	for (size_t i = 0; i < newVertexBuffers.size(); i++)
	{
		delete[] newVertexBuffers[i];
	}

	TRDE::destroyModel(model);
}

void Converter::GenerateBoneRemapTable(const TRDE::MeshGroup& currentMeshGroup, const TRDE::FaceGroup& faceGroup, const TRDE::VertexDeclarationHeader& vertDeclHeader, const char* vertexBuffer, unsigned short* indexBuffer, TRDE::VertexDeclaration* skinIndexDecl, std::vector<std::vector<int>>& boneRemapTables, std::vector<TRDE::MeshGroup>& newMeshGroups, std::vector<TRDE::FaceGroup>& newFaceGroups, std::vector<unsigned short*>& newIndexBuffers, std::vector<char*>& newVertexBuffers, std::vector<TRDE::VertexDeclarationHeader>& newVertexDeclarationHeaders, std::vector<TRDE::VertexDeclaration>& vertDecls, std::vector<std::vector<TRDE::VertexDeclaration>>& newVertexDeclarations)
{
	char* tempVertexBuffer = new char[vertDeclHeader.m_vertexStride*currentMeshGroup.m_numVertices];
	std::memcpy(tempVertexBuffer, vertexBuffer, vertDeclHeader.m_vertexStride*currentMeshGroup.m_numVertices);

	bool* vertexVisitationTable = new bool[currentMeshGroup.m_numVertices];
	std::memset(vertexVisitationTable, false, currentMeshGroup.m_numVertices * sizeof(bool));

	std::vector<int> boneRemapTable;
	unsigned int lastSplit = 0;
	unsigned short maxFaceIndex = 0;
	unsigned short minFaceIndex = USHRT_MAX;
	
	for (unsigned int i = 0; i < faceGroup.m_numFaces * 3; i++)
	{
		unsigned short vertexIndex = indexBuffer[i];

		if (minFaceIndex > vertexIndex)
		{
			minFaceIndex = vertexIndex;
		}

		if (maxFaceIndex < vertexIndex)
		{
			maxFaceIndex = vertexIndex;
		}

		unsigned char* vertex = (unsigned char*)tempVertexBuffer + (vertDeclHeader.m_vertexStride*(indexBuffer[i])) + skinIndexDecl->m_position;

		//Sometimes index buffers may use the same vertex more than once.
		//Here we prevent this with a vertex visitation table.
		if (!vertexVisitationTable[vertexIndex])
		{
			for (unsigned char j = 0; j < 4; j++)
			{
				*(vertex + j) = addToBoneRemapTable(boneRemapTable, *(vertex + j));
				assert(*(vertex + j) < boneRemapTable.size());
			}

			vertexVisitationTable[vertexIndex] = true;
		}

#if 1
		//TODO: should not be > invalid case!
		if (boneRemapTable.size() + 2 >= MAX_BONES_PER_MESH_GROUP && !(i%3))
		{
			std::cout << "Warning: face group is using too many bones! Expected <= " << MAX_BONES_PER_MESH_GROUP << " Got: " << boneRemapTable.size() << std::endl;

			boneRemapTables.push_back(boneRemapTable);
			boneRemapTable.clear();

			//Add new mesh group
			TRDE::MeshGroup newMeshGroup = currentMeshGroup;
			newMeshGroup.m_numFaceGroups = 1;
			TRDE::FaceGroup newFaceGroup = faceGroup;
			newFaceGroup.m_numFaces = ((i - lastSplit)/3);

			newVertexBuffers.push_back(GenerateVertexBuffer(newFaceGroup.m_numFaces, indexBuffer + lastSplit, tempVertexBuffer, vertDeclHeader.m_vertexStride, newMeshGroup.m_numVertices, maxFaceIndex));

			newMeshGroups.push_back(newMeshGroup);
			newFaceGroups.push_back(newFaceGroup);
			newIndexBuffers.push_back(indexBuffer + lastSplit);
			newVertexDeclarationHeaders.push_back(vertDeclHeader);
			newVertexDeclarations.push_back(vertDecls);
			std::memcpy(tempVertexBuffer, vertexBuffer, vertDeclHeader.m_vertexStride*currentMeshGroup.m_numVertices);
			std::memset(reinterpret_cast<char*>(vertexVisitationTable), false, currentMeshGroup.m_numVertices * sizeof(bool));

			lastSplit = i;
		}
#endif
	}

	if (lastSplit == 0)
	{
		boneRemapTables.push_back(boneRemapTable);
		boneRemapTable.clear();

		//Add new mesh group
		TRDE::MeshGroup newMeshGroup = currentMeshGroup;
		newMeshGroup.m_numFaceGroups = 1;
		TRDE::FaceGroup newFaceGroup = faceGroup;
		newVertexBuffers.push_back(GenerateVertexBuffer(newFaceGroup.m_numFaces, indexBuffer + lastSplit, tempVertexBuffer, vertDeclHeader.m_vertexStride, newMeshGroup.m_numVertices, maxFaceIndex));


		newVertexDeclarationHeaders.push_back(vertDeclHeader);
		newVertexDeclarations.push_back(vertDecls);
		newMeshGroups.push_back(newMeshGroup);
		newFaceGroups.push_back(newFaceGroup);
		newIndexBuffers.push_back(indexBuffer);
	}
	else
	{
#if 1
		boneRemapTables.push_back(boneRemapTable);
		boneRemapTable.clear();

		//Add new mesh group
		TRDE::MeshGroup newMeshGroup = currentMeshGroup;
		newMeshGroup.m_numFaceGroups = 1;
		TRDE::FaceGroup newFaceGroup = faceGroup;
		newFaceGroup.m_numFaces = (((faceGroup.m_numFaces * 3) - lastSplit) / 3);
		newVertexBuffers.push_back(GenerateVertexBuffer(newFaceGroup.m_numFaces, indexBuffer + lastSplit, tempVertexBuffer, vertDeclHeader.m_vertexStride, newMeshGroup.m_numVertices, maxFaceIndex));

		newMeshGroups.push_back(newMeshGroup);
		newFaceGroups.push_back(newFaceGroup);
		newIndexBuffers.push_back(indexBuffer + lastSplit);
		newVertexDeclarationHeaders.push_back(vertDeclHeader);
		newVertexDeclarations.push_back(vertDecls);
#endif
	}

	delete[] vertexVisitationTable;
	delete[] tempVertexBuffer;
}

//Returns the index into the bone remap table if the bone index already exists, otherwise adds it.
unsigned char Converter::addToBoneRemapTable(std::vector<int>& boneRemapTable, const unsigned char& boneIndex)
{
	for (size_t i = 0; i < boneRemapTable.size(); i++)//TODO: Iterate end to begin
	{
		if (boneRemapTable[i] == boneIndex)
		{
			//Already in bone remap table, no need to add the bone index, just return it!
			return i;
		}
	}

	//Bone index is not present in the bone remap table, add it then return the index into the bone remap table.
	boneRemapTable.push_back(boneIndex);
	return static_cast<unsigned char>(boneRemapTable.size() - 1);
}

//Compares each TRDE bone to every single TRAS bone and generates a global bone remap table used to map TRAS skel to TRDE model.
void Converter::GenerateGlobalBoneRemapTable(std::vector<int>& boneRemapTable, std::vector<TRAS::Bone>& trasBones, std::vector<TRDE::Bone>& trdeBones)
{
	bool* boneVisitationTable = new bool[trasBones.size()];
	std::memset(boneVisitationTable, false, sizeof(bool) * trasBones.size());

	for (size_t i = 0; i < trdeBones.size(); i++)
	{
		float lowestDistance = 0.0f;
		int closestBoneIndex = 0;

#if _DEBUG
		bool bFoundBoneMatch = false;
#endif
		TRDE::Bone& currentBone = trdeBones[i];

		for (size_t j = 0; j < trasBones.size(); j++)
		{
			TRAS::Bone& compareBone = trasBones[j];
			
			//Found exact bone positional match, keep it!
			if (currentBone.m_pos[0] == compareBone.m_pos[0] && currentBone.m_pos[1] == compareBone.m_pos[1] && currentBone.m_pos[2] == compareBone.m_pos[2] &&!boneVisitationTable[j])
			{
				closestBoneIndex = j;

#if _DEBUG
				bFoundBoneMatch = true;
				boneVisitationTable[closestBoneIndex] = true;
#endif
				
				break;
			}

			//Just in-case we don't find a perfect bone positional match, store which bone is closest to the current TRDE bone.
			float currentDistance = GetDistanceBetweenBones(currentBone, compareBone);
			if (currentDistance < lowestDistance || j == 0)
			{
				lowestDistance = currentDistance;
				closestBoneIndex = j;
			}
		}

#if _DEBUG
		if(!bFoundBoneMatch)
		{
			//If trUE SAFE TO MOVE BONE TO DEFINITIVE EDITION POS
			std::cout << "Warning: Did not find bone match, bone was remapped to closest! Dist:" << lowestDistance << " Should Move: " << !boneVisitationTable[closestBoneIndex] << " TRAS Closest Index: " << closestBoneIndex <<  " TRDE Index:" << i << std::endl;
			std::cout << "X: " << trasBones[closestBoneIndex].m_pos[0] << "Y: " << trasBones[closestBoneIndex].m_pos[1] << "Z: " << trasBones[closestBoneIndex].m_pos[2] << std::endl;
			std::cout << "X: " << trdeBones[i].m_pos[0] << "Y: " << trdeBones[i].m_pos[1] << "Z: " << trdeBones[i].m_pos[2] << std::endl;

		}
#endif
		boneRemapTable.push_back(closestBoneIndex);
	}

	delete[] boneVisitationTable;
}


//When we split meshes, we must re-index both the index and vertex buffer, so they're relative to 0.
char* Converter::GenerateVertexBuffer(unsigned int numFaces, unsigned short* indexBuffer, char* vertexBuffer, unsigned int vertexStride, unsigned long long& resultVertexCount, unsigned short maxVertexIndex)
{
	//HACK: Since we copy/duplicate all mesh group properties, we must hack the vertex count back to 0. This prevents incorrect initial vertex count whilst avoiding two vars and a copy back to resultVertexCount.
	resultVertexCount = 0;

	//If this has occurred, revert the commit that merged vertex visitation table and face index remap table for optimisation purposes.
	assert(maxVertexIndex < USHRT_MAX);

	unsigned short* indexBufferRemapTable = new unsigned short[maxVertexIndex+1];
	std::memset((char*)indexBufferRemapTable, USHRT_MAX, (maxVertexIndex+1) * sizeof(unsigned short));//TODO remove cast

	char* remappedVertexBuffer = new char[(maxVertexIndex + 1)*vertexStride];

	for (unsigned int i = 0; i < numFaces * 3; i++)
	{
		//Vertex not already remapped! Remap it!
		if (indexBufferRemapTable[indexBuffer[i]] == USHRT_MAX)
		{		
			//Update face index remap table
			indexBufferRemapTable[indexBuffer[i]] = resultVertexCount;

			//Copy the vertex to the new vertex buffer to remapped position.
			std::memcpy(remappedVertexBuffer + (resultVertexCount*vertexStride), vertexBuffer + (vertexStride*indexBuffer[i]), vertexStride);
			
			//Used for tracking reconstructedVertexBuffer's currentVertexIndex so we can remap the vertex buffer (copy to current vertex index on next iter)
			resultVertexCount++;
		}

		//Remap the current vertex index
		*(&indexBuffer[i]) = indexBufferRemapTable[indexBuffer[i]];
	}

	delete[] indexBufferRemapTable;

#if _DEBUG && 0
	//Checks for out of range vertex indices
	for (unsigned int i = 0; i < numFaces * 3; i++)
	{
		unsigned short* test = &indexBuffer[i];
		assert(indexBuffer[i] < resultVertexCount);
	}
#endif

	return remappedVertexBuffer;
}

//Returns the length as float between one TRDE bone and a TRAS bone. Used to determine which TRAS bone is closest to a TRDE bone for weight transferring. 
__inline float Converter::GetDistanceBetweenBones(TRDE::Bone& bone, TRAS::Bone& trasBone)
{
	return std::sqrtf((trasBone.m_pos[0] - bone.m_pos[0]) * (trasBone.m_pos[0] - bone.m_pos[0])) + ((trasBone.m_pos[1] - bone.m_pos[1]) * (trasBone.m_pos[1] - bone.m_pos[1])) + ((trasBone.m_pos[2] - bone.m_pos[2]) * (trasBone.m_pos[2] - bone.m_pos[2]));
}

//Credit: UnpackTRU (Dunsan), this is customised for C++ with endian swap.
void Converter::decodeNormal(char* normalPointer)
{
#if !ENDIAN_BIG
	unsigned char reversedNormal[4];
	reversedNormal[0] = *(unsigned char*)(normalPointer + 3);
	reversedNormal[1] = *(unsigned char*)(normalPointer + 2);
	reversedNormal[2] = *(unsigned char*)(normalPointer + 1);
	reversedNormal[3] = *(unsigned char*)(normalPointer + 0);
#endif

	int extractedNormal[3];
	float convertedNormal[3];
	for (unsigned char i = 0; i < 3; i++)
	{
		extractedNormal[i] = 0;
		for (int j = 0; j < 10; j++)
		{
			extractedNormal[i] |= ExtractBit(reversedNormal, (9 - j) + ((i*10)+2)) << j;
		}

		if (extractedNormal[i] >= 512)
		{
			extractedNormal[i] -= 1024;
		}

#if ENDIAN_BIG
		convertedNormal[i] = (static_cast<float>(extractedNormal[i]) / 511.0f);
#else
		convertedNormal[2-i] = (static_cast<float>(extractedNormal[i]) / 511.0f);
#endif
	}

	//Normalise
	float length = std::sqrtf(convertedNormal[0] * convertedNormal[0] + convertedNormal[1] * convertedNormal[1] + convertedNormal[2] * convertedNormal[2]);
	for (unsigned int i = 0; i < 3; i++)
	{
		normalPointer[i] = static_cast<unsigned char>((((convertedNormal[i] / length) * 127.0f) + 127.0f));
	}

	//Sign
	normalPointer[3] = SCHAR_MAX;
}

//Credit: UnpackTRU (Dunsan), this is customised for C++ with endian swap.
int Converter::ExtractBit(unsigned char* bytes, int bitIndex)
{
	int bit = (bytes[bitIndex >> 3] & (1 << (7 - bitIndex % 8))) != 0 ? 1 : 0;
	return bit;
}