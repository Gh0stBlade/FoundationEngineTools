#include "TRAS_Model.h"

#include <iostream>
#include <fstream>
#include <cassert>

void TRAS::loadModel(TRAS::Model& model, std::ifstream& stream)
{
	unsigned int numRelocations;
	stream.read(reinterpret_cast<char*>(&numRelocations), sizeof(numRelocations));
	stream.seekg(16, SEEK_SET);

	unsigned int numRelocations2;
	stream.read(reinterpret_cast<char*>(&numRelocations2), sizeof(numRelocations2));
	stream.seekg(24, SEEK_SET);

	unsigned int meshStart;
	stream.read(reinterpret_cast<char*>(&meshStart), sizeof(meshStart));

	stream.seekg(((20 + numRelocations * 8) + numRelocations2 * 4) + meshStart, SEEK_SET);
	long meshBase = stream.tellg();

	stream.read(reinterpret_cast<char*>(&model.m_meshHeader), sizeof(TRAS::MeshHeader));

	//We must declare this variable to track the current face group later on.
	unsigned int currentFaceGroup = 0;

	for (unsigned short i = 0; i < model.m_meshHeader.m_numMeshGroups; i++)
	{
		stream.seekg(meshBase + model.m_meshHeader.m_offsetMeshGroups + i * sizeof(TRAS::MeshGroup), SEEK_SET);
		model.m_meshGroups.emplace_back();
		const TRAS::MeshGroup& currentMeshGroup = model.m_meshGroups.back();
		stream.read(reinterpret_cast<char*>(&model.m_meshGroups.back()), sizeof(TRAS::MeshGroup));

		//Reading the vertex buffer.
		char* vertexBuffer;
		if (currentMeshGroup.m_numVertices > 0)
		{
			stream.seekg(meshBase + currentMeshGroup.m_offsetFVFInfo, SEEK_SET);
			TRAS::VertexDeclarationHeader vertDeclHeader;
			stream.read(reinterpret_cast<char*>(&vertDeclHeader), sizeof(TRAS::VertexDeclarationHeader));

			///Load vertex buffer into memory now
			stream.seekg(meshBase + currentMeshGroup.m_offsetVertexBuffer, SEEK_SET);
			vertexBuffer = new char[currentMeshGroup.m_numVertices * vertDeclHeader.m_vertexStride];
			stream.read(vertexBuffer, currentMeshGroup.m_numVertices * vertDeclHeader.m_vertexStride);

			//Parse out the vertexComponents
			stream.seekg(meshBase + currentMeshGroup.m_offsetFVFInfo + sizeof(TRAS::VertexDeclarationHeader), SEEK_SET);

			//Bind vertex components
			for (unsigned short j = 0; j < vertDeclHeader.m_componentCount; j++)
			{
				TRAS::VertexDeclaration vertDecl;
				stream.read(reinterpret_cast<char*>(&vertDecl), sizeof(TRAS::VertexDeclaration));

				switch (vertDecl.m_componentNameHashed)
				{
				case 0xD2F7D823://Position
					break;
				case 0x36F5E414://Normal
					break;
				case 0x3E7F6149://TessellationNormal
					break;
				case 0xF1ED11C3://Tangent
					break;
				case 0x64A86F01://Binormal
					break;
				case 0x9B1D4EA://PackedNTB
					break;
				case 0x48E691C0://SkinWeights
					break;
				case 0x5156D8D3://SkinIndices
					break;
				case 0x7E7DD623://Color1
					break;
				case 0x733EF0FA://Color2
					break;
				case 0x8317902A://Texcoord1
					break;
				case 0x8E54B6F3://Texcoord2
					break;
				case 0x8A95AB44://Texcoord3
					break;
				case 0x94D2FB41://Texcoord4
					break;
				case 0xE7623ECF://InstanceID
					break;
				default:
					break;
				}
			}
		}

		//Load face groups
		for (unsigned int j = 0; j < currentMeshGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			stream.seekg(meshBase + model.m_meshHeader.m_offsetFaceGroups + currentFaceGroup * sizeof(TRAS::FaceGroup), SEEK_SET);
			model.m_faceGroups.emplace_back();
			stream.read(reinterpret_cast<char*>(&model.m_faceGroups.back()), sizeof(TRAS::FaceGroup));

			const TRAS::FaceGroup& faceGroup = model.m_faceGroups.back();

			//Load index buffer and constuct model
			stream.seekg(meshBase + model.m_meshHeader.m_offsetFaceBuffer + faceGroup.m_offsetFaceStart, SEEK_SET);
			char* faceBuff = new char[faceGroup.m_numFaces*6];
			stream.read(faceBuff, faceGroup.m_numFaces * 6);

			//Destroy index buffer
			delete[] faceBuff;
		}

		//Delete vertex buffer.
		if (currentMeshGroup.m_numVertices > 0)
		{
			delete[] vertexBuffer;
		}
	}
}

void TRAS::loadSkeleton(TRAS::Skeleton & skeleton)
{
	std::ifstream inputStream2("lara.trasmesh", std::ios::binary);
	if (!inputStream2.good())
	{
		std::cout << "Failed to open TRAS Model file!" << std::endl;
		assert(false);
		return;
	}

	//Read TRAS Bones
	unsigned int numRelocations;
	inputStream2.read(reinterpret_cast<char*>(&numRelocations), sizeof(unsigned int));
	inputStream2.seekg(16, SEEK_SET);

	unsigned int numRelocations2;
	inputStream2.read(reinterpret_cast<char*>(&numRelocations2), sizeof(unsigned int));
	inputStream2.seekg(24, SEEK_SET);

	inputStream2.seekg(((numRelocations * 8) + 4), SEEK_SET);
	unsigned int offsetBoneInfo;
	unsigned int offsetBoneInfo2;
	inputStream2.read(reinterpret_cast<char*>(&offsetBoneInfo), sizeof(unsigned int));
	inputStream2.read(reinterpret_cast<char*>(&offsetBoneInfo2), sizeof(unsigned int));

	inputStream2.seekg(((20 + numRelocations * 8) + numRelocations2 * 4), SEEK_SET);
	long meshBase = inputStream2.tellg();

	inputStream2.seekg(meshBase + offsetBoneInfo - 4, SEEK_SET);

	unsigned int numBones;
	inputStream2.read(reinterpret_cast<char*>(&numBones), sizeof(unsigned int));
	inputStream2.seekg(12, SEEK_CUR);

	inputStream2.seekg(meshBase + offsetBoneInfo2, SEEK_SET);
	printf("%i\n", (unsigned int)inputStream2.tellg());
	///@FIXME need to add to get world bone pos
	for (unsigned int i = 0; i < numBones; i++)
	{
		TRAS::Bone bone;
		inputStream2.seekg(0x20, SEEK_CUR);
		inputStream2.read(reinterpret_cast<char*>(&bone.m_pos[0]), sizeof(float));
		inputStream2.read(reinterpret_cast<char*>(&bone.m_pos[1]), sizeof(float));
		inputStream2.read(reinterpret_cast<char*>(&bone.m_pos[2]), sizeof(float));
		inputStream2.seekg(12, SEEK_CUR);
		int parent;
		inputStream2.read(reinterpret_cast<char*>(&parent), sizeof(int));
		if (skeleton.m_bones.size() > parent)
		{
			bone.m_pos[0] += skeleton.m_bones[parent].m_pos[0];
			bone.m_pos[1] += skeleton.m_bones[parent].m_pos[1];
			bone.m_pos[2] += skeleton.m_bones[parent].m_pos[2];
		}
		inputStream2.seekg(4, SEEK_CUR);
		skeleton.m_bones.push_back(bone);
	}

	inputStream2.close();
}
