#pragma once

#include <fstream>

#include "TRDE_Model.h"
#include "TRAS_Model.h"

#include "VectorMath.h"
#include <vector>

enum kVertexAttributeTypes
{
	POSITION,
	NORMAL,
	TESSELLATION_NORMAL,
	TANGENT,
	BI_NORMAL,
	PACKED_NTB,
	SKIN_WEIGHTS,
	SKIN_INDICES,
	COLOR_1,
	COLOR_2,
	TEXCOORD1,
	TEXCOORD2,
	TEXCOORD3,
	TEXCOORD4,
	INSTANCE_ID,
	MAX_NUM_VERTEX_ATTRIBUTES
};

struct VertexDeclarationList
{
	VertexDeclarationList()
	{
		for (unsigned int i = 0; i < kVertexAttributeTypes::MAX_NUM_VERTEX_ATTRIBUTES; i++)
		{
			this->m_vertDecls[i] = nullptr;
		}
	}

	~VertexDeclarationList()
	{
		for (unsigned int i = 0; i < kVertexAttributeTypes::MAX_NUM_VERTEX_ATTRIBUTES; i++)
		{
			this->m_vertDecls[i] = nullptr;
		}
	}

	TRDE::VertexDeclaration* m_vertDecls[MAX_NUM_VERTEX_ATTRIBUTES];
};



namespace Converter
{
	void Convert(std::ifstream& inStream, std::ofstream& outStream);
	void GenerateBoneRemapTable(const TRDE::VertexGroup& currentVertexGroup, const TRDE::FaceGroup& faceGroup, const TRDE::VertexDeclarationHeader& vertDeclHeader, const char* vertexBuffer, unsigned short* indexBuffer, const VertexDeclarationList& vertDeclList, std::vector<std::vector<int>>& boneRemapTables, std::vector<TRDE::VertexGroup>& newMeshGroups, std::vector<TRDE::FaceGroup>& newFaceGroups, std::vector<unsigned short*>& newIndexBuffers, std::vector<char*>& newVertexBuffers, std::vector<TRDE::VertexDeclarationHeader>& newVertexDeclarationHeaders, std::vector<TRDE::VertexDeclaration>& vertDecls, std::vector<std::vector<TRDE::VertexDeclaration>>& newVertexDeclarations);
	unsigned char addToBoneRemapTable(std::vector<int>& boneRemapTable, const unsigned char& boneIndex);
	float GetDistanceBetweenBones(const TRDE::Bone& bone, const TRAS::Bone& trasBone);
	void GenerateGlobalBoneRemapTable(std::vector<int>& boneRemapTable, std::vector<TRAS::Bone>& trasBones, std::vector<TRDE::Bone>& trdeBones);
	char* GenerateVertexBuffer(unsigned int numTris, unsigned short* indexBuffer, char* vertexBuffer, unsigned int vertexStride, unsigned long long& resultVertexCount, unsigned short maxVertexIndex, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList);
	void decodeNormal(char* normalPointer);
	int ExtractBit(unsigned char* bytes, int bitIndex);
	unsigned int alignOffset(const unsigned int& offset, const unsigned char& alignment);
	void RecalculateTangents(const unsigned long& vertexCount, unsigned int vertexStride, const unsigned int numTriangles, char* vertexBuffer, unsigned short* indexBuffer, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList);
	void UpdateTangents(char* vertexBuffer, unsigned int numVertices, unsigned int vertexStride, std::vector<Vector3>& tangents, std::vector<Vector3>& bitangents, const VertexDeclarationList& vertDeclList);
};