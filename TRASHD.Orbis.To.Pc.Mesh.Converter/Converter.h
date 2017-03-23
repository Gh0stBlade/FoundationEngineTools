#pragma once

#include <fstream>

#include "TRDE_Model.h"
#include "TRAS_Model.h"

namespace Converter
{
	void Convert(std::ifstream& inStream, std::ofstream& outStream);
	void GenerateBoneRemapTable(const TRDE::MeshGroup& currentMeshGroup, const TRDE::FaceGroup& faceGroup, const TRDE::VertexDeclarationHeader& vertDeclHeader, const char* vertexBuffer, unsigned short* indexBuffer, TRDE::VertexDeclaration* skinIndexDecl, std::vector<std::vector<int>>& boneRemapTables, std::vector<TRDE::MeshGroup>& newMeshGroups, std::vector<TRDE::FaceGroup>& newFaceGroups, std::vector<unsigned short*>& newIndexBuffers, std::vector<char*>& newVertexBuffers, std::vector<TRDE::VertexDeclarationHeader>& newVertexDeclarationHeaders, std::vector<TRDE::VertexDeclaration>& vertDecls, std::vector<std::vector<TRDE::VertexDeclaration>>& newVertexDeclarations);
	unsigned char addToBoneRemapTable(std::vector<int>& boneRemapTable, const unsigned char& boneIndex);
	float GetDistanceBetweenBones(TRDE::Bone& bone, TRAS::Bone& trasBone);
	void GenerateGlobalBoneRemapTable(std::vector<int>& boneRemapTable, std::vector<TRAS::Bone>& trasBones, std::vector<TRDE::Bone>& trdeBones);
	char* GenerateVertexBuffer(unsigned int numFaces, unsigned short* indexBuffer, char* vertexBuffer, unsigned int vertexStride, unsigned long long& resultVertexCount, unsigned short maxVertexIndex);
	void decodeNormal(char* normalPointer);
	int ExtractBit(unsigned char* bytes, int bitIndex);
};