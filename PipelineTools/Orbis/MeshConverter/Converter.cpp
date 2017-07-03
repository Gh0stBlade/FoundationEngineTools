#include "Converter.h"
#include "TRDE_Model.h"
#include "TRAS_Model.h"

#include "VectorMath.h"

#include <cassert>
#include <iostream>

#define USHRT_MAX 0xFFFF
#define SCHAR_MAX 127

const unsigned int MAX_BONES_PER_MESH_GROUP = 42;
const unsigned char NUM_BONE_INFLUENCES_PER_VERTEX = 4;

void Converter::Convert(std::ifstream& inStream, std::ofstream& outStream)
{
	TRDE::Model model;
	TRDE::loadModel(model, inStream);
	
	std::vector<TRDE::VertexGroup> newVertexGroups;
	std::vector<TRDE::FaceGroup> newFaceGroups;
	std::vector<std::vector<int>> boneRemapTables;
	std::vector<unsigned short*> newIndexBuffers;
	std::vector<char*> newVertexBuffers;
	std::vector<TRDE::VertexDeclarationHeader> newVertexDeclarationHeaders;
	std::vector<std::vector<TRDE::VertexDeclaration>> newVertexDeclarations;

	//Setup our vertexDeclList, will allow us to easily convert specific vertex attribute data later on without the use of excessive loops.
	

	//Used for tracking current processing face group.
	unsigned int currentFaceGroup = 0;

	for (unsigned int i = 0; i < model.m_vertexGroups.size(); i++)
	{
		TRDE::VertexGroup& currentVertexGroup = model.m_vertexGroups[i];

		//Get the skin indices vertex decl info
		TRDE::VertexDeclarationHeader& vertDeclHeader = model.m_vertexDeclarationHeaders[i];
		std::vector<TRDE::VertexDeclaration>& vertDecls = model.m_vertexDeclarations[i];

		VertexDeclarationList vertDeclList;

		for (unsigned int j = 0; j < vertDeclHeader.m_componentCount; j++)
		{
			TRDE::VertexDeclaration* vertDecl = &vertDecls[j];

			switch (vertDecl->m_componentNameHashed)
			{
			case 0xD2F7D823://Position
				vertDeclList.m_vertDecls[kVertexAttributeTypes::POSITION] = vertDecl;
				break;
			case 0x36F5E414://Normal
				vertDeclList.m_vertDecls[kVertexAttributeTypes::NORMAL] = vertDecl;
				break;
			case 0x3E7F6149://TessellationNormal
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TESSELLATION_NORMAL] = vertDecl;
				break;
			case 0xF1ED11C3://Tangent
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TANGENT] = vertDecl;
				break;
			case 0x64A86F01://Binormal
				vertDeclList.m_vertDecls[kVertexAttributeTypes::BI_NORMAL] = vertDecl;
				break;
			case 0x9B1D4EA://PackedNTB
				vertDeclList.m_vertDecls[kVertexAttributeTypes::PACKED_NTB] = vertDecl;
				break;
			case 0x48E691C0://SkinWeights
				vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_WEIGHTS] = vertDecl;
				break;
			case 0x5156D8D3://SkinIndices
				vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_INDICES] = vertDecl;
				break;
			case 0x7E7DD623://Color1
				vertDeclList.m_vertDecls[kVertexAttributeTypes::COLOR_1] = vertDecl;
				break;
			case 0x733EF0FA://Color2
				vertDeclList.m_vertDecls[kVertexAttributeTypes::COLOR_2] = vertDecl;
				break;
			case 0x8317902A://Texcoord1
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD1] = vertDecl;
				break;
			case 0x8E54B6F3://Texcoord2
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD2] = vertDecl;
				break;
			case 0x8A95AB44://Texcoord3
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD3] = vertDecl;
				break;
			case 0x94D2FB41://Texcoord4
				vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD4] = vertDecl;
				break;
			case 0xE7623ECF://InstanceID
				vertDeclList.m_vertDecls[kVertexAttributeTypes::INSTANCE_ID] = vertDecl;
				break;
			default:
				break;
			}
		}

		//Tomb Raider Definitive Edition (PS4) Mesh files have no bone remap table.
		//Tomb Raider (PC) Mesh files do have bone remap tables. Each "mesh group" supports a maximum of 42 bones for skinning.
		//Here we split PS4 face/mesh groups accordingly.
		//Note: We only need to do it for skinned meshes
		if (vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_WEIGHTS] != nullptr && vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_INDICES] != nullptr)//Todo mesh header flag i think it means static = 2, skinned/skeletal = 0, tress fx = 1
		{
			//For each face group in this mesh group, let's generate the bone remap table and new split, vertex buffers.
			for (unsigned int j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
			{
				TRDE::FaceGroup& faceGroup = model.m_faceGroups[currentFaceGroup];
				Converter::GenerateBoneRemapTable(currentVertexGroup, faceGroup, vertDeclHeader, model.m_vertexBuffers[i], reinterpret_cast<unsigned short*>(model.m_indexBuffers[currentFaceGroup]), vertDeclList, boneRemapTables, newVertexGroups, newFaceGroups, newIndexBuffers, newVertexBuffers, newVertexDeclarationHeaders, vertDecls, newVertexDeclarations);
			}
		}
		else
		{
			newVertexDeclarationHeaders.push_back(vertDeclHeader);
			newVertexDeclarations.push_back(vertDecls);
			newVertexGroups.push_back(currentVertexGroup);

			newVertexBuffers.push_back(model.m_vertexBuffers[i]);
			for (unsigned int j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
			{
				TRDE::FaceGroup& faceGroup = model.m_faceGroups[currentFaceGroup];
				newFaceGroups.push_back(faceGroup);
				newIndexBuffers.push_back((unsigned short*)model.m_indexBuffers[currentFaceGroup]);
			}
		}
	}

	//Load TRAS/TRDE skeleton
	TRDE::Skeleton skeleton;
	TRAS::Skeleton trasSkel;
	if (model.m_meshHeader.m_meshType == TRDE::SKELETAL_MESH)
	{
		TRDE::loadSkeleton(skeleton);
		TRAS::loadSkeleton(trasSkel);
	}

	//+16 unknown? lod info?
	unsigned int offsetBoneIndexList = alignOffset(sizeof(TRAS::MeshHeader), 15) + 16;
	outStream.seekp(offsetBoneIndexList, SEEK_SET);

	//Write bone index list, not sure what this is used for yet.
	for (size_t i = 0; i < trasSkel.m_bones.size(); i++)
	{
		outStream.write(reinterpret_cast<char*>(&i), sizeof(unsigned int));
	}

	//We won't write the mesh groups now because we don't have enough information about the vertex buffers
	unsigned int offsetVertexGroups = alignOffset(offsetBoneIndexList + (sizeof(unsigned int) * trasSkel.m_bones.size()), 15);

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

	unsigned int offsetBoneRemapTables = offsetVertexGroups + newVertexGroups.size() * sizeof(TRAS::VertexGroup);
	outStream.seekp(offsetBoneRemapTables, SEEK_SET);

	//Writing the bone remap tables.
	for (size_t i = 0; i < boneRemapTables.size(); i++)
	{
		for (size_t j = 0; j < boneRemapTables[i].size(); j++)
		{
			outStream.write(reinterpret_cast<char*>(&boneRemapTables[i][j]), sizeof(int));
		}

		outStream.seekp(alignOffset(outStream.tellp(), 15), SEEK_SET);
	}

	//Writing vertex declaration header, decls and vertex buffers
	for (size_t i = 0; i < newVertexGroups.size(); i++)
	{
		TRDE::VertexGroup& currentMeshGroup = newVertexGroups[i];
		TRDE::VertexDeclarationHeader& vertDeclHeader = newVertexDeclarationHeaders[i];

		currentMeshGroup.m_offsetFVFInfo = static_cast<unsigned int>(outStream.tellp());
		outStream.write(reinterpret_cast<char*>(&newVertexDeclarationHeaders[i]), sizeof(TRDE::VertexDeclarationHeader));

		for (size_t j = 0; j < newVertexDeclarations[i].size(); j++)
		{
			outStream.write(reinterpret_cast<char*>(&newVertexDeclarations[i][j]), sizeof(TRDE::VertexDeclaration));
		}
		
		char* vertexBuffer = newVertexBuffers[i];
		currentMeshGroup.m_offsetVertexBuffer = static_cast<unsigned int>(outStream.tellp());

		outStream.write(vertexBuffer, vertDeclHeader.m_vertexStride*currentMeshGroup.m_numVertices);
		outStream.seekp(alignOffset(outStream.tellp(), 15), SEEK_SET);
		outStream.seekp(16, SEEK_CUR);
	}

	unsigned int offsetIndexBuffer = static_cast<unsigned int>(outStream.tellp());
	std::vector<unsigned int> indexBufferOffsets;
	
	//Write Index buffers
	currentFaceGroup = 0;
	for (size_t i = 0; i < newVertexGroups.size(); i++)
	{
		TRDE::VertexGroup& currentVertexGroup = newVertexGroups[i];
		
		for (size_t j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			TRDE::FaceGroup& faceGroup = newFaceGroups[currentFaceGroup];

			indexBufferOffsets.push_back(static_cast<unsigned int>(outStream.tellp()) - offsetIndexBuffer);
			char* faceBuff = reinterpret_cast<char*>(newIndexBuffers[currentFaceGroup]);

			switch (model.m_meshHeader.m_meshType)
			{
			case TRDE::kMeshType::TRESS_FX:
				outStream.write(faceBuff, (faceGroup.m_numTris * 2) * sizeof(unsigned short));
				break;
			case TRDE::kMeshType::SKELETAL_MESH:
			case TRDE::kMeshType::STATIC_MESH:
				outStream.write(faceBuff, (faceGroup.m_numTris * 3) * sizeof(unsigned short));
				break;
			}
		}
	}

	//Writing Face groups
	unsigned int offsetFaceGroups = alignOffset(outStream.tellp(), 15) + 16;
	outStream.seekp(offsetFaceGroups, SEEK_SET);

	currentFaceGroup = 0;
	for (size_t i = 0; i < newVertexGroups.size(); i++)
	{
		TRDE::VertexGroup& currentVertexGroup = newVertexGroups[i];
		
		for (size_t j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			newFaceGroups[currentFaceGroup].m_indexBufferStartIndex = indexBufferOffsets[currentFaceGroup]/2;
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

	//Writing the mesh header
	outStream.seekp(0, SEEK_SET);
	model.m_meshHeader.m_fileVersion = 0x0E01;
	outStream.write(reinterpret_cast<char*>(&model.m_meshHeader), 112);

	outStream.seekp(116, SEEK_SET);
	outStream.write(reinterpret_cast<char*>(&offsetFaceGroups), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetVertexGroups), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetBoneIndexList), sizeof(unsigned int));
	outStream.write(reinterpret_cast<char*>(&offsetBoneIndexList), sizeof(unsigned int));//Lodinfo!
	outStream.write(reinterpret_cast<char*>(&offsetIndexBuffer), sizeof(unsigned int));
	unsigned short numFaceGroups = static_cast<unsigned short>(newFaceGroups.size());
	outStream.write(reinterpret_cast<char*>(&numFaceGroups), sizeof(unsigned short));
	unsigned short numMeshGroups = static_cast<unsigned short>(newVertexGroups.size());
	outStream.write(reinterpret_cast<char*>(&numMeshGroups), sizeof(unsigned short));

	unsigned short numBones = trasSkel.m_bones.size();
	outStream.write(reinterpret_cast<char*>(&numBones), sizeof(unsigned short));
	outStream.seekp(4, SEEK_CUR);//Num lods

	//Write updated vertex groups
	outStream.seekp(offsetVertexGroups, SEEK_SET);
	for (size_t i = 0; i < newVertexGroups.size(); i++)
	{
		TRDE::VertexGroup& currentVertexGroup = newVertexGroups[i];
		unsigned int test = boneRemapTables.size() > 0 ? boneRemapTables[i].size() : 0;//TODO if skeletal mesh flag
		outStream.write(reinterpret_cast<char*>(&currentVertexGroup.m_numFaceGroups), sizeof(unsigned int));
		outStream.write(reinterpret_cast<char*>(&currentVertexGroup.m_lodIndex), sizeof(unsigned short));
		outStream.write(reinterpret_cast<char*>(&test), sizeof(unsigned short));
		
		outStream.write(reinterpret_cast<char*>(&offsetBoneRemapTables), sizeof(unsigned int));
		offsetBoneRemapTables += test * sizeof(unsigned int);
		offsetBoneRemapTables = (((offsetBoneRemapTables + 15) & ~15));

		outStream.write(reinterpret_cast<char*>(&currentVertexGroup.m_offsetVertexBuffer), sizeof(unsigned int));
		outStream.seekp(12, SEEK_CUR);//Padding
		outStream.write(reinterpret_cast<char*>(&currentVertexGroup.m_offsetFVFInfo), sizeof(unsigned int));
		outStream.write(reinterpret_cast<char*>(&currentVertexGroup.m_numVertices), sizeof(unsigned int));
		outStream.seekp(4, SEEK_CUR);//Padding
		outStream.seekp(8, SEEK_CUR);
	}

	for (size_t i = 0; i < newVertexBuffers.size(); i++)
	{
		delete[] newVertexBuffers[i];
	}

	TRDE::destroyModel(model);
}

void Converter::GenerateBoneRemapTable(const TRDE::VertexGroup& currentVertexGroup, const TRDE::FaceGroup & faceGroup, const TRDE::VertexDeclarationHeader & vertDeclHeader, const char* vertexBuffer, unsigned short* indexBuffer, const VertexDeclarationList & vertDeclList, std::vector<std::vector<int>>& boneRemapTables, std::vector<TRDE::VertexGroup>& newVertexGroups, std::vector<TRDE::FaceGroup>& newFaceGroups, std::vector<unsigned short*>& newIndexBuffers, std::vector<char*>& newVertexBuffers, std::vector<TRDE::VertexDeclarationHeader>& newVertexDeclarationHeaders, std::vector<TRDE::VertexDeclaration>& vertDecls, std::vector<std::vector<TRDE::VertexDeclaration>>& newVertexDeclarations)
{
	char* tempVertexBuffer = new char[vertDeclHeader.m_vertexStride*currentVertexGroup.m_numVertices];
	std::memcpy(tempVertexBuffer, vertexBuffer, vertDeclHeader.m_vertexStride*currentVertexGroup.m_numVertices);

	bool* vertexVisitationTable = new bool[currentVertexGroup.m_numVertices];
	std::memset(vertexVisitationTable, false, currentVertexGroup.m_numVertices * sizeof(bool));

	std::vector<int> boneRemapTable;
	unsigned int lastSplit = 0;

	//Used for tracking how many vertices have currently been processed
	unsigned short maxVertexIndex = 0;

	for (unsigned int i = 0; i < faceGroup.m_numTris * 3; i++)
	{
		unsigned short vertexIndex = indexBuffer[i];

		if (maxVertexIndex < vertexIndex)
		{
			maxVertexIndex = vertexIndex;
		}

		//Convert PS4 vertex attributes to PC compatible.
		//Sometimes index buffers may use the same vertex more than once.
		//Here we prevent remapping the same vertex twice with a vertex visitation table.
		if (!vertexVisitationTable[vertexIndex])
		{
			if (vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_INDICES] != nullptr)
			{
				unsigned char* vertexSkinIndices = (unsigned char*) tempVertexBuffer + (vertDeclHeader.m_vertexStride*(indexBuffer[i])) + vertDeclList.m_vertDecls[kVertexAttributeTypes::SKIN_INDICES]->m_position;
				for (unsigned char j = 0; j < NUM_BONE_INFLUENCES_PER_VERTEX; j++)
				{
					*(vertexSkinIndices + j) = addToBoneRemapTable(boneRemapTable, *(vertexSkinIndices + j));
					assert(*(vertexSkinIndices + j) < boneRemapTable.size());
				}
			}

			if (vertDeclList.m_vertDecls[kVertexAttributeTypes::NORMAL] != nullptr)
			{
				char* vertexNormals = tempVertexBuffer + (vertDeclHeader.m_vertexStride*(indexBuffer[i])) + vertDeclList.m_vertDecls[kVertexAttributeTypes::NORMAL]->m_position;
				decodeNormal(vertexNormals);
			}

			if (vertDeclList.m_vertDecls[kVertexAttributeTypes::TESSELLATION_NORMAL] != nullptr)
			{
				char* vertexTesselationNormals = tempVertexBuffer + (vertDeclHeader.m_vertexStride*(indexBuffer[i])) + vertDeclList.m_vertDecls[kVertexAttributeTypes::TESSELLATION_NORMAL]->m_position;
				std::memset(vertexTesselationNormals, 0, 16);
			}

			vertexVisitationTable[vertexIndex] = true;
		}
		
		if (boneRemapTable.size() + 2 >= MAX_BONES_PER_MESH_GROUP && !(i % 3))
		{
			std::cout << "Warning: face group is using too many bones! Expected <= " << MAX_BONES_PER_MESH_GROUP << " Got: " << boneRemapTable.size() << std::endl;

			boneRemapTables.push_back(boneRemapTable);
			boneRemapTable.clear();

			//Add new mesh group
			TRDE::VertexGroup newVertexGroup = currentVertexGroup;
			newVertexGroup.m_numFaceGroups = 1;
			TRDE::FaceGroup newFaceGroup = faceGroup;
			newFaceGroup.m_numTris = ((i - lastSplit) / 3);

			std::vector<Vector3> tangents;
			std::vector<Vector3> bitangents;

			newVertexBuffers.push_back(GenerateVertexBuffer(newFaceGroup.m_numTris, indexBuffer + lastSplit, tempVertexBuffer, vertDeclHeader.m_vertexStride, newVertexGroup.m_numVertices, maxVertexIndex, tangents, bitangents, vertDeclList));
			UpdateTangents(newVertexBuffers.back(), tangents.size(), vertDeclHeader.m_vertexStride, tangents, bitangents, vertDeclList);
			tangents.clear();
			bitangents.clear();
			newVertexGroups.push_back(newVertexGroup);
			newFaceGroups.push_back(newFaceGroup);
			newIndexBuffers.push_back(indexBuffer + lastSplit);
			newVertexDeclarationHeaders.push_back(vertDeclHeader);
			newVertexDeclarations.push_back(vertDecls);
			std::memcpy(tempVertexBuffer, vertexBuffer, vertDeclHeader.m_vertexStride*currentVertexGroup.m_numVertices);
			std::memset(reinterpret_cast<char*>(vertexVisitationTable), false, currentVertexGroup.m_numVertices * sizeof(bool));

			lastSplit = i;
		}
	}

	boneRemapTables.push_back(boneRemapTable);
	boneRemapTable.clear();

	//Add new mesh group
	TRDE::VertexGroup newVertexGroup = currentVertexGroup;
	newVertexGroup.m_numFaceGroups = 1;
	TRDE::FaceGroup newFaceGroup = faceGroup;

	if (lastSplit == 0)
	{
		newIndexBuffers.push_back(indexBuffer);
	}
	else
	{
		newFaceGroup.m_numTris = (((faceGroup.m_numTris * 3) - lastSplit) / 3);
		newIndexBuffers.push_back(indexBuffer + lastSplit);
	}

	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	newVertexBuffers.push_back(GenerateVertexBuffer(newFaceGroup.m_numTris, indexBuffer + lastSplit, tempVertexBuffer, vertDeclHeader.m_vertexStride, newVertexGroup.m_numVertices, maxVertexIndex, tangents, bitangents, vertDeclList));
	UpdateTangents(newVertexBuffers.back(), tangents.size(), vertDeclHeader.m_vertexStride, tangents, bitangents, vertDeclList);
	tangents.clear();
	bitangents.clear();

	newVertexDeclarationHeaders.push_back(vertDeclHeader);
	newVertexDeclarations.push_back(vertDecls);
	newVertexGroups.push_back(newVertexGroup);
	newFaceGroups.push_back(newFaceGroup);

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
			return static_cast<unsigned char>(i);
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

		const TRDE::Bone& currentBone = trdeBones[i];

		if (i < trasBones.size())
		{
			if (currentBone.m_pos[0] == trasBones[i].m_pos[0] && currentBone.m_pos[1] == trasBones[i].m_pos[1] && currentBone.m_pos[2] == trasBones[i].m_pos[2])
			{
				closestBoneIndex = i;
				boneRemapTable.push_back(closestBoneIndex);
				continue;
			}
		}
		
		for (size_t j = 0; j < trasBones.size(); j++)
		{
			const TRAS::Bone& compareBone = trasBones[j];

			//Found exact bone positional match, keep it!
			if (currentBone.m_pos[0] == compareBone.m_pos[0] && currentBone.m_pos[1] == compareBone.m_pos[1] && currentBone.m_pos[2] == compareBone.m_pos[2] && currentBone.m_parentIndex == compareBone.m_parentIndex)// && !boneVisitationTable[j])
			{
				closestBoneIndex = j;
				boneVisitationTable[closestBoneIndex] = true;
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

		boneRemapTable.push_back(closestBoneIndex);
	}

	delete [] boneVisitationTable;
}


//When we split meshes, we must re-index both the index and vertex buffer, so they're relative to 0.
char* Converter::GenerateVertexBuffer(unsigned int numTris, unsigned short* indexBuffer, char* vertexBuffer, unsigned int vertexStride, unsigned long long& resultVertexCount, unsigned short maxVertexIndex, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList)
{
	//HACK: Since we copy/duplicate all mesh group properties, we must hack the vertex count back to 0. This prevents incorrect initial vertex count whilst avoiding two vars and a copy back to resultVertexCount.
	resultVertexCount = 0;

	//If this has occurred, revert the commit that merged vertex visitation table and index buffer remap table for optimisation purposes.
	assert(maxVertexIndex < USHRT_MAX);

	unsigned short* indexBufferRemapTable = new unsigned short[maxVertexIndex+1];
	std::memset(indexBufferRemapTable, USHRT_MAX, (maxVertexIndex+1) * sizeof(unsigned short));

	char* remappedVertexBuffer = new char[(maxVertexIndex + 1)*vertexStride];

	for (unsigned int i = 0; i < numTris * 3; i++)
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
	for (unsigned int i = 0; i < numTris * 3; i++)
	{
		unsigned short* test = &indexBuffer[i];
		assert(indexBuffer[i] < resultVertexCount);
	}
#endif

	RecalculateTangents(resultVertexCount, vertexStride, numTris, remappedVertexBuffer, indexBuffer, tangents, bitangents, vertDeclList);


	return remappedVertexBuffer;
}

void Converter::RecalculateTangents(const unsigned long& vertexCount, unsigned int vertexStride, const unsigned int numTriangles, char* vertexBuffer, unsigned short* indexBuffer, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList)
{
	if (vertexBuffer != nullptr && indexBuffer != nullptr)
	{
		std::vector<Vector3> tan1;
		std::vector<Vector3> tan2;
		std::vector<float> tan_w;
		for (unsigned int i = 0; i < vertexCount; i++)
		{
			tan1.push_back(Vector3(0.0f, 0.0f, 0.0f));
			tan2.push_back(Vector3(0.0f, 0.0f, 0.0f));
			tangents.push_back(Vector3(0.0f, 0.0f, 0.0f));
			bitangents.push_back(Vector3(0.0f, 0.0f, 0.0f));
			tan_w.push_back(0.0f);
		}

		for(int i = 0; i < numTriangles; i++)
		{
			unsigned short i1 = indexBuffer[i];
			unsigned short i2 = indexBuffer[i+1];
			unsigned short i3 = indexBuffer[i+2];

			float* fv1 = (float*)(vertexBuffer + (vertexStride*i1) + vertDeclList.m_vertDecls[kVertexAttributeTypes::POSITION]->m_position);
			float* fv2 = (float*)(vertexBuffer + (vertexStride*i2) + vertDeclList.m_vertDecls[kVertexAttributeTypes::POSITION]->m_position);
			float* fv3 = (float*)(vertexBuffer + (vertexStride*i3) + vertDeclList.m_vertDecls[kVertexAttributeTypes::POSITION]->m_position);

			Vector3 v1(*fv1, *(fv1 + 1), *(fv1 + 2));
			Vector3 v2(*fv2, *(fv2 + 1), *(fv2 + 2));
			Vector3 v3(*fv3, *(fv3 + 1), *(fv3 + 2));

			short* sw1 = (short*) (vertexBuffer + (vertexStride*i1) + vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD1]->m_position);
			short* sw2 = (short*) (vertexBuffer + (vertexStride*i2) + vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD1]->m_position);
			short* sw3 = (short*) (vertexBuffer + (vertexStride*i3) + vertDeclList.m_vertDecls[kVertexAttributeTypes::TEXCOORD1]->m_position);

			Vector3 w1((*sw1) / 2048.0f, (*sw1 + 1) / 2048.0f, 0.0f);
			Vector3 w2((*sw2) / 2048.0f, (*sw2 + 1) / 2048.0f, 0.0f);
			Vector3 w3((*sw3) / 2048.0f, (*sw3 + 1) / 2048.0f, 0.0f);

			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;

			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;

			float divr = (s1 * t2 - s2 * t1);
			if (divr == 0.0f)
			{
				divr = 0.001f;
			}
				
			float r = 1.0f / divr;

			Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

			tan1[i1] = tan1[i1] + sdir;
			tan1[i2] = tan1[i2] + sdir;
			tan1[i3] = tan1[i3] + sdir;

			tan2[i1] = tan2[i1] + tdir;
			tan2[i2] = tan2[i2] + tdir;
			tan2[i3] = tan2[i3] + tdir;
		}

		for (int i = 0; i < vertexCount; i++)
		{
			unsigned char* normalPointer = reinterpret_cast<unsigned char*>(&vertexBuffer[(vertexStride * i) + vertDeclList.m_vertDecls[kVertexAttributeTypes::NORMAL]->m_position]);
#if 1
			Vector3 n = Vector3((static_cast<float>(normalPointer[0]) / 255.0f * 2 - 1), (static_cast<float>(normalPointer[1]) / 255.0f * 2 - 1), (static_cast<float>(normalPointer[2]) / 255.0f * 2 - 1));
			n.normalize();
#else
			Vector3 n = Vector3((static_cast<float>(normalPointer[0]) - 127.0f / 127.0f), (static_cast<float>(normalPointer[1]) - 127.0f / 127.0f), (static_cast<float>(normalPointer[2]) - 127.0f / 127.0f));
			n.normalize();
#endif
			Vector3 t = tan1[i];

			tangents[i] = (t - n * n.dot(t));
			tangents[i].normalize();

			tan_w[i] = (n.cross(t).dot(tan2[i]) < 0.0f) ? -1.0f : 1.0f;

			bitangents[i] = n.cross(tangents[i]) * tan_w[i];
		}
	}
	else
	{
		std::cout << "Illegal vertex or index buffer!" << std::endl;
	}
}

void Converter::UpdateTangents(char* vertexBuffer, unsigned int numVertices, unsigned int vertexStride, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList)
{

	for (int i = 0; i < numVertices; i++)
	{
		if (vertDeclList.m_vertDecls[kVertexAttributeTypes::TANGENT] != nullptr)
		{
			char* tangentPointer = vertexBuffer + (vertexStride*i) + vertDeclList.m_vertDecls[kVertexAttributeTypes::TANGENT]->m_position;

			*tangentPointer = static_cast<char>((((tangents[i].x) * 127.0f) + 127.0f));
			*(tangentPointer + 1) = static_cast<char>((((tangents[i].y) * 127.0f) + 127.0f));
			*(tangentPointer + 2) = static_cast<char>((((tangents[i].z) * 127.0f) + 127.0f));
			*(tangentPointer + 3) = 0;
		}

		if (vertDeclList.m_vertDecls[kVertexAttributeTypes::BI_NORMAL] != nullptr)
		{
			char* biNormalPointer = vertexBuffer + (vertexStride*i) + vertDeclList.m_vertDecls[kVertexAttributeTypes::BI_NORMAL]->m_position;

			*biNormalPointer = static_cast<char>((((bitangents[i].x) * 127.0f) + 127.0f));
			*(biNormalPointer + 1) = static_cast<char>((((bitangents[i].y) * 127.0f) + 127.0f));
			*(biNormalPointer + 2) = static_cast<char>((((bitangents[i].z) * 127.0f) + 127.0f));
			*(biNormalPointer + 3) = 0;
		}
	}
}

//Returns the length as float between one TRDE bone and a TRAS bone. Used to determine which TRAS bone is closest to a TRDE bone for weight transferring. 
__inline float Converter::GetDistanceBetweenBones(const TRDE::Bone& bone, const TRAS::Bone& trasBone)
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
			extractedNormal[i] |= ExtractBit(reversedNormal, (9 - j) + ((i * 10) + 2)) << j;
		}

		if (extractedNormal[i] >= 512)
		{
			extractedNormal[i] -= 1024;
		}

		convertedNormal[2 - i] = (static_cast<float>(extractedNormal[i]) / 511.0f);
	}

	//Normalise
	float length = std::sqrtf(convertedNormal[0] * convertedNormal[0] + convertedNormal[1] * convertedNormal[1] + convertedNormal[2] * convertedNormal[2]);
	for (unsigned int i = 0; i < 3; i++)
	{
		normalPointer[i] = static_cast<char>((((convertedNormal[i]) * 127.0f) + 127.0f));
	}

	normalPointer[3] = static_cast<unsigned char>(0);
}

//Credit: UnpackTRU (Dunsan), this is customised for C++ with endian swap.
int Converter::ExtractBit(unsigned char* bytes, int bitIndex)
{
	return (bytes[bitIndex >> 3] & (1 << (7 - bitIndex % 8))) != 0 ? 1 : 0;
}

unsigned int Converter::alignOffset(const unsigned int& offset, const unsigned char& alignment)
{
	return (offset + alignment) & ~alignment;
}