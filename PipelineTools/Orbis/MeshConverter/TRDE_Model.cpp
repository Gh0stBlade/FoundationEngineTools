#include "TRDE_Model.h"
#include <fstream>
#include <iostream>
#include <cassert>

/*
* TRDefinitiveEdition::loadModel()
* Reads Tomb Raider: Definitive Edition model file and loads the data directly into Shift Engine.
*/
void TRDE::loadModel(TRDE::Model& model, std::ifstream& stream)
{
	stream.read(reinterpret_cast<char*>(&model.m_meshHeader), sizeof(TRDE::MeshHeader));

#if 0 //Not sure what this data does, maybe used for lod dist constraints.
	io.seek(meshHeader.m_offsetLodInfo + i * sizeof(TRDefinitiveEdition::LodInfo), SEEK_ABSOLUTE);
	for (unsigned int i = 0; i < meshHeader.m_numLods; i++)
	{
		TRAscension::LodInfo lodInfo;
		io.read(reinterpret_cast<char*>(&lodInfo), sizeof(TRDefinitiveEdition::LodInfo));
	}
#endif

	//We must declare this variable to track the current face group being processed later on.
	unsigned int currentFaceGroup = 0;
	
	for (unsigned short i = 0; i < model.m_meshHeader.m_numVertexGroups; i++)
	{
		stream.seekg(model.m_meshHeader.m_offsetVertexGroups + i * sizeof(TRDE::VertexGroup), SEEK_SET);

		model.m_vertexGroups.emplace_back();
		TRDE::VertexGroup& currentVertexGroup = model.m_vertexGroups.back();
		stream.read(reinterpret_cast<char*>(&currentVertexGroup), sizeof(TRDE::VertexGroup));
		
		if (currentVertexGroup.m_numVertices == 0)
			continue;

		stream.seekg(currentVertexGroup.m_offsetFVFInfo, SEEK_SET);
		model.m_vertexDeclarationHeaders.emplace_back();

		TRDE::VertexDeclarationHeader& vertDeclHeader = model.m_vertexDeclarationHeaders.back();
		stream.read(reinterpret_cast<char*>(&vertDeclHeader), sizeof(TRDE::VertexDeclarationHeader));

		stream.seekg(currentVertexGroup.m_offsetVertexBuffer, SEEK_SET);
		char* vertexBuffer = new char[currentVertexGroup.m_numVertices * vertDeclHeader.m_vertexStride];
		model.m_vertexBuffers.emplace_back(vertexBuffer);

		stream.read(vertexBuffer, (currentVertexGroup.m_numVertices * vertDeclHeader.m_vertexStride));

		stream.seekg(currentVertexGroup.m_offsetFVFInfo + sizeof(TRDE::VertexDeclarationHeader), SEEK_SET);

		std::vector<TRDE::VertexDeclaration> vertDecls;

		for (unsigned short j = 0; j < vertDeclHeader.m_componentCount; j++)
		{
			vertDecls.emplace_back();
			TRDE::VertexDeclaration& vertDecl = vertDecls.back();
			stream.read(reinterpret_cast<char*>(&vertDecl), sizeof(TRDE::VertexDeclaration));

		}

		//Reorder to maintain FVF
		if(model.m_meshHeader.m_meshType != kMeshType::TRESS_FX)
			vertDecls = ReorderVertDecls(vertDecls, currentVertexGroup, vertexBuffer, vertDeclHeader);

		model.m_vertexDeclarations.push_back(vertDecls);

		for (unsigned int j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			stream.seekg(model.m_meshHeader.m_offsetFaceGroups + currentFaceGroup * sizeof(TRDE::FaceGroup), SEEK_SET);

			model.m_faceGroups.emplace_back();
			TRDE::FaceGroup& faceGroup = model.m_faceGroups.back();
			stream.read(reinterpret_cast<char*>(&faceGroup), sizeof(TRDE::FaceGroup));

			if (faceGroup.m_numTris == 0)
				continue;

			stream.seekg(model.m_meshHeader.m_offsetFaceBuffer + (faceGroup.m_indexBufferStartIndex * sizeof(unsigned short)), SEEK_SET);
			char* indexBuff = nullptr;

			switch (model.m_meshHeader.m_meshType)
			{
			case TRDE::kMeshType::SKELETAL_MESH:
			case TRDE::kMeshType::STATIC_MESH:
			{
				indexBuff = new char[(faceGroup.m_numTris * 3) * sizeof(unsigned short)];
				stream.read(indexBuff, (faceGroup.m_numTris * 3) * sizeof(unsigned short));
				model.m_indexBuffers.emplace_back(indexBuff);
				break;
			}
			case TRDE::kMeshType::TRESS_FX:
			{
				indexBuff = new char[(faceGroup.m_numTris * 2) * sizeof(unsigned short)];
				stream.read(indexBuff, (faceGroup.m_numTris * 2) * sizeof(unsigned short));
				model.m_indexBuffers.emplace_back(indexBuff);
				break;
			}
			default:
				assert(false);
				break;
			}
		}
	}
}

void TRDE::loadSkeleton(TRDE::Skeleton& skeleton)
{

	std::ifstream inputStream3("skeleton.trdemesh", std::ios::binary);

	if (!inputStream3.good())
	{
		std::cout << "Failed to open TRDE Skeleton file!" << std::endl;
		assert(false);
		return;
	}

	//Read TRDE Bones
	unsigned int offsetBoneInfo = 0;
	unsigned int offsetBoneInfo2 = 0;
	unsigned int meshBase = 0;
	unsigned int numBones = 0;

	int numRelocations;
	inputStream3.read(reinterpret_cast<char*>(&numRelocations), sizeof(unsigned int));
	inputStream3.seekg(16, SEEK_SET);

	int numRelocations2;
	inputStream3.read(reinterpret_cast<char*>(&numRelocations2), sizeof(unsigned int));

	inputStream3.seekg(((numRelocations * 8) + 4), SEEK_SET);
	inputStream3.read(reinterpret_cast<char*>(&offsetBoneInfo), sizeof(unsigned int));
	inputStream3.read(reinterpret_cast<char*>(&offsetBoneInfo2), sizeof(unsigned int));

	inputStream3.seekg(((28 + numRelocations * 8) + numRelocations2 * 4), SEEK_SET);
	meshBase = static_cast<unsigned int>(inputStream3.tellg());

	inputStream3.seekg(meshBase + offsetBoneInfo - 8, SEEK_SET);

	inputStream3.read(reinterpret_cast<char*>(&numBones), sizeof(unsigned int));
	inputStream3.seekg(meshBase + offsetBoneInfo2, SEEK_SET);

	for (unsigned int i = 0; i < numBones; i++)
	{
		TRDE::Bone bone;

		inputStream3.seekg(0x20, SEEK_CUR);
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[0]), sizeof(float));
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[1]), sizeof(float));
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[2]), sizeof(float));
		unsigned short temp = 0xFFFF;
		inputStream3.seekg(12, SEEK_CUR);
		inputStream3.read(reinterpret_cast<char*>(&bone.m_parentIndex), sizeof(int));
		inputStream3.seekg(20, SEEK_CUR);
		skeleton.m_bones.push_back(bone);
	}

	for (unsigned int i = 1; i < numBones; i++)
	{
		Bone* currentBone = &skeleton.m_bones[i];
		if (currentBone->m_parentIndex > -1 && currentBone->m_parentIndex < skeleton.m_bones.size())
		{
			currentBone->m_parentBone = &skeleton.m_bones[currentBone->m_parentIndex];
			currentBone->m_pos[0] += currentBone->m_parentBone->m_pos[0];
			currentBone->m_pos[1] += currentBone->m_parentBone->m_pos[1];
			currentBone->m_pos[2] += currentBone->m_parentBone->m_pos[2];
		}
		else
		{
			assert(false);
		}
	}

	inputStream3.close();
}

std::vector<TRDE::VertexDeclaration> TRDE::ReorderVertDecls(std::vector<TRDE::VertexDeclaration>& vertDecl, TRDE::VertexGroup& currentVertexGroup, char* vertexBuffer, TRDE::VertexDeclarationHeader& vertDeclHeader)
{
	std::vector<TRDE::VertexDeclaration> reorderedVertexDecls;
	std::vector<unsigned char> componentOffsetList;
	std::vector<unsigned short> componentSize;
	unsigned char currentOffset = 0;
	unsigned short vertexStride = 0;
	std::vector<int> oldVertDeclPositions;

	//Initial fvf patch
	vertDeclHeader.m_unk00 = 0x715F18AB;
	vertDeclHeader.m_unk01 = 0x31E15EBB;

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0xD2F7D823)//Position
		{
			vertDecl[i].m_unk00 = 2;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(12);
			vertexStride += 12;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x48E691C0)//SkinWeights
		{
			vertDecl[i].m_unk00 = 0x6;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x5156D8D3)//SkinIndices
		{
			vertDecl[i].m_unk00 = 7;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x36F5E414)//Normal
		{
			vertDecl[i].m_unk00 = 5;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}


	//DUMMY DATA
	TRDE::VertexDeclaration tessVertDecl;
	tessVertDecl.m_componentNameHashed = 0x3E7F6149;
	tessVertDecl.m_unk00 = 3;
	
	reorderedVertexDecls.push_back(tessVertDecl);
	componentOffsetList.push_back(currentOffset);
	componentSize.push_back(16);
	vertexStride += 16;
	oldVertDeclPositions.push_back(0);

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x8317902A)//TexCoord
		{
			vertDecl[i].m_unk00 = 0x13;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x8E54B6F3)//TexCoord2
		{
			//HACK
			vertDeclHeader.m_unk00 = 0xF18AB156;
			vertDeclHeader.m_unk01 = 0xD93268A2;

			vertDecl[i].m_unk00 = 0x13;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0x64A86F01)//BiNormal
		{
			vertDecl[i].m_unk00 = 0x5;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	for (size_t i = 0; i < vertDecl.size(); i++)
	{
		if (vertDecl[i].m_componentNameHashed == 0xF1ED11C3)//Tangent
		{
			vertDecl[i].m_unk00 = 0x5;
			reorderedVertexDecls.push_back(vertDecl[i]);
			componentOffsetList.push_back(currentOffset);
			componentSize.push_back(4);
			vertexStride += 4;
			oldVertDeclPositions.push_back(vertDecl[i].m_position);
			break;
		}
	}

	unsigned int currentOffsetNew = 0;
	char* tempVertexBuffer = new char[currentVertexGroup.m_numVertices * vertexStride];
	

	for (unsigned int j = 0; j < currentVertexGroup.m_numVertices; j++)
	{
		currentOffsetNew = 0;

		for (size_t i = 0; i < reorderedVertexDecls.size(); i++)
		{
 			std::memcpy(tempVertexBuffer + (j*vertexStride) + currentOffsetNew, vertexBuffer + (j*vertDeclHeader.m_vertexStride) + oldVertDeclPositions[i], componentSize[i]);
			reorderedVertexDecls[i].m_position = currentOffsetNew;
			currentOffsetNew += componentSize[i];
		}
	}

	std::memcpy(vertexBuffer, tempVertexBuffer, currentVertexGroup.m_numVertices * vertexStride);


	

	delete[] tempVertexBuffer;

	vertDeclHeader.m_componentCount = static_cast<unsigned short>(reorderedVertexDecls.size());
	vertDeclHeader.m_vertexStride = vertexStride;

	return reorderedVertexDecls;
}

void TRDE::destroyModel(TRDE::Model & model)
{
	for (unsigned int i = 0; i < model.m_vertexBuffers.size(); i++)
	{
		delete[] model.m_vertexBuffers[i];
	}

	for (unsigned int i = 0; i < model.m_indexBuffers.size(); i++)
	{
		delete[] model.m_indexBuffers[i];
	}
}
