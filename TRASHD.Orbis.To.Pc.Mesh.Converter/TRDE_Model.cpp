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

	for (unsigned short i = 0; i < model.m_meshHeader.m_numMeshGroups; i++)
	{
		stream.seekg(model.m_meshHeader.m_offsetMeshGroups + i * sizeof(TRDE::MeshGroup), SEEK_SET);

		model.m_meshGroups.emplace_back();
		TRDE::MeshGroup& currentMeshGroup = model.m_meshGroups.back();
		stream.read(reinterpret_cast<char*>(&currentMeshGroup), sizeof(TRDE::MeshGroup));

		if (currentMeshGroup.m_numVertices == 0)
			continue;

		stream.seekg(currentMeshGroup.m_offsetFVFInfo, SEEK_SET);
		model.m_vertexDeclarationHeaders.emplace_back();

		TRDE::VertexDeclarationHeader& vertDeclHeader = model.m_vertexDeclarationHeaders.back();
		stream.read(reinterpret_cast<char*>(&vertDeclHeader), sizeof(TRDE::VertexDeclarationHeader));

		stream.seekg(currentMeshGroup.m_offsetVertexBuffer, SEEK_SET);
		char* vertexBuffer = new char[currentMeshGroup.m_numVertices * vertDeclHeader.m_vertexStride];
		model.m_vertexBuffers.emplace_back(vertexBuffer);

		stream.read(vertexBuffer, (currentMeshGroup.m_numVertices * vertDeclHeader.m_vertexStride));

		stream.seekg(currentMeshGroup.m_offsetFVFInfo + sizeof(TRDE::VertexDeclarationHeader), SEEK_SET);

		std::vector<TRDE::VertexDeclaration> vertDecls;

		for (unsigned short j = 0; j < vertDeclHeader.m_componentCount; j++)
		{
			vertDecls.emplace_back();
			TRDE::VertexDeclaration& vertDecl = vertDecls.back();
			stream.read(reinterpret_cast<char*>(&vertDecl), sizeof(TRDE::VertexDeclaration));

		}

		if(model.m_meshHeader.m_primitiveDrawType > 0)
			vertDecls = ReorderVertDecls(vertDecls, currentMeshGroup, vertexBuffer, vertDeclHeader);

		model.m_vertexDeclarations.push_back(vertDecls);

		for (unsigned int j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			stream.seekg(model.m_meshHeader.m_offsetFaceGroups + currentFaceGroup * sizeof(TRDE::FaceGroup), SEEK_SET);

			model.m_faceGroups.emplace_back();
			TRDE::FaceGroup& faceGroup = model.m_faceGroups.back();
			stream.read(reinterpret_cast<char*>(&faceGroup), sizeof(TRDE::FaceGroup));

			if (faceGroup.m_numFaces == 0)
				continue;

			stream.seekg(model.m_meshHeader.m_offsetFaceBuffer + (faceGroup.m_offsetFaceStart * sizeof(unsigned short)), SEEK_SET);
			char* faceBuff = nullptr;

			switch (model.m_meshHeader.m_primitiveDrawType)
			{
			case TRDE::kPrimitiveDrawType::TRIANGLE_LIST:
			case 2:
			{
				faceBuff = new char[(faceGroup.m_numFaces * 3) * sizeof(unsigned short)];
				stream.read(faceBuff, (faceGroup.m_numFaces * 3) * sizeof(unsigned short));
				model.m_indexBuffers.emplace_back(faceBuff);
				break;
			}
			case TRDE::kPrimitiveDrawType::LINE_LIST:
			{
				faceBuff = new char[(faceGroup.m_numFaces * 2) * sizeof(unsigned short)];
				stream.read(faceBuff, (faceGroup.m_numFaces * 2) * sizeof(unsigned short));
				model.m_indexBuffers.emplace_back(faceBuff);
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
	std::ofstream outStream("bones.bin", std::ios::binary);

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
	meshBase = inputStream3.tellg();

	inputStream3.seekg(meshBase + offsetBoneInfo - 8, SEEK_SET);

	inputStream3.read(reinterpret_cast<char*>(&numBones), sizeof(unsigned int));
	inputStream3.seekg(meshBase + offsetBoneInfo2, SEEK_SET);
	printf("%i\n", (unsigned int)inputStream3.tellg());
	for (unsigned int i = 0; i < numBones; i++)
	{
		TRDE::Bone bone;

		outStream.seekp(0x20, SEEK_CUR);
		
		inputStream3.seekg(0x20, SEEK_CUR);
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[0]), sizeof(float));
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[1]), sizeof(float));
		inputStream3.read(reinterpret_cast<char*>(&bone.m_pos[2]), sizeof(float));
		outStream.write(reinterpret_cast<char*>(&bone.m_pos[0]), sizeof(float));
		outStream.write(reinterpret_cast<char*>(&bone.m_pos[1]), sizeof(float));
		outStream.write(reinterpret_cast<char*>(&bone.m_pos[2]), sizeof(float));
		outStream.seekp(10, SEEK_CUR);
		unsigned short temp = 0xFFFF;
		outStream.write(reinterpret_cast<char*>(&temp), sizeof(unsigned short));
		inputStream3.seekg(12, SEEK_CUR);
		int parent;
		inputStream3.read(reinterpret_cast<char*>(&parent), sizeof(int));
		outStream.write(reinterpret_cast<char*>(&parent), sizeof(int));
		outStream.seekp(4, SEEK_CUR);
		if (skeleton.m_bones.size() > parent)
		{
			bone.m_pos[0] += skeleton.m_bones[parent].m_pos[0];
			bone.m_pos[1] += skeleton.m_bones[parent].m_pos[1];
			bone.m_pos[2] += skeleton.m_bones[parent].m_pos[2];
		}
		inputStream3.seekg(20, SEEK_CUR);
		skeleton.m_bones.push_back(bone);
	}

	inputStream3.close();

	outStream.close();
}

std::vector<TRDE::VertexDeclaration> TRDE::ReorderVertDecls(std::vector<TRDE::VertexDeclaration>& vertDecl, TRDE::MeshGroup & currentMeshGroup, char* vertexBuffer, TRDE::VertexDeclarationHeader& vertDeclHeader)
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
	char* tempVertexBuffer = new char[currentMeshGroup.m_numVertices * vertexStride];
	

	for (unsigned int j = 0; j < currentMeshGroup.m_numVertices; j++)
	{
		currentOffsetNew = 0;

		for (size_t i = 0; i < reorderedVertexDecls.size(); i++)
		{
 			std::memcpy(tempVertexBuffer + (j*vertexStride) + currentOffsetNew, vertexBuffer + (j*vertDeclHeader.m_vertexStride) + oldVertDeclPositions[i], componentSize[i]);
			reorderedVertexDecls[i].m_position = currentOffsetNew;
			currentOffsetNew += componentSize[i];
		}
	}

	std::memcpy(vertexBuffer, tempVertexBuffer, currentMeshGroup.m_numVertices * vertexStride);


	

	delete[] tempVertexBuffer;

	vertDeclHeader.m_componentCount = reorderedVertexDecls.size();
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
