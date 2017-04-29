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
	unsigned int meshBase = static_cast<unsigned int>(stream.tellg());

	stream.read(reinterpret_cast<char*>(&model.m_meshHeader), sizeof(TRAS::MeshHeader));

	//We must declare this variable to track the current face group later on.
	unsigned int currentFaceGroup = 0;

	for (unsigned short i = 0; i < model.m_meshHeader.m_numVertexGroups; i++)
	{
		stream.seekg(meshBase + model.m_meshHeader.m_offsetVertexGroups + i * sizeof(TRAS::VertexGroup), SEEK_SET);
		model.m_vertexGroups.emplace_back();
		const TRAS::VertexGroup& currentVertexGroup = model.m_vertexGroups.back();
		stream.read(reinterpret_cast<char*>(&model.m_vertexGroups.back()), sizeof(TRAS::VertexGroup));

		//Reading the vertex buffer.
		char* vertexBuffer;
		if (currentVertexGroup.m_numVertices > 0)
		{
			stream.seekg(meshBase + currentVertexGroup.m_offsetFVFInfo, SEEK_SET);
			TRAS::VertexDeclarationHeader vertDeclHeader;
			stream.read(reinterpret_cast<char*>(&vertDeclHeader), sizeof(TRAS::VertexDeclarationHeader));

			///Load vertex buffer into memory now
			stream.seekg(meshBase + currentVertexGroup.m_offsetVertexBuffer, SEEK_SET);
			vertexBuffer = new char[currentVertexGroup.m_numVertices * vertDeclHeader.m_vertexStride];
			stream.read(vertexBuffer, currentVertexGroup.m_numVertices * vertDeclHeader.m_vertexStride);

			//Parse out the vertexComponents
			stream.seekg(meshBase + currentVertexGroup.m_offsetFVFInfo + sizeof(TRAS::VertexDeclarationHeader), SEEK_SET);
		}

		//Load face groups
		for (unsigned int j = 0; j < currentVertexGroup.m_numFaceGroups; j++, currentFaceGroup++)
		{
			stream.seekg(meshBase + model.m_meshHeader.m_offsetFaceGroups + currentFaceGroup * sizeof(TRAS::FaceGroup), SEEK_SET);
			model.m_faceGroups.emplace_back();
			stream.read(reinterpret_cast<char*>(&model.m_faceGroups.back()), sizeof(TRAS::FaceGroup));

			const TRAS::FaceGroup& faceGroup = model.m_faceGroups.back();

			//Load index buffer and constuct model
			stream.seekg(meshBase + model.m_meshHeader.m_offsetFaceBuffer + faceGroup.m_indexBufferStartIndex, SEEK_SET);
			char* faceBuff = new char[(faceGroup.m_numTris * 3) * sizeof(unsigned short)];
			stream.read(faceBuff, (faceGroup.m_numTris * 3) * sizeof(unsigned short));

			//Destroy index buffer
			delete[] faceBuff;
		}

		//Delete vertex buffer.
		if (currentVertexGroup.m_numVertices > 0)
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
	unsigned int meshBase = static_cast<unsigned int>(inputStream2.tellg());

	inputStream2.seekg(meshBase + offsetBoneInfo - 4, SEEK_SET);

	unsigned int numBones;
	inputStream2.read(reinterpret_cast<char*>(&numBones), sizeof(unsigned int));
	inputStream2.seekg(12, SEEK_CUR);

	inputStream2.seekg(meshBase + offsetBoneInfo2, SEEK_SET);
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
