#pragma once

#include <string>
#include <vector>

namespace TRDE
{
#pragma pack(push, 1)

	struct VertexDeclaration
	{
		unsigned int m_componentNameHashed;
		unsigned short m_position;
		unsigned short m_unk00;//maybe data type, unconfirmed
	};

	struct VertexDeclarationHeader
	{
		unsigned int m_unk00;//fvf info
		unsigned int m_unk01;
		unsigned short m_componentCount;
		unsigned short m_vertexStride;
		unsigned int m_padding;
	};

	struct FaceGroup
	{
		float m_unk00[4];

		unsigned int m_offsetFaceStart;
		unsigned int m_numFaces;
		unsigned int m_unk01;
		unsigned int m_unk02;

		long long m_unk03;
		long long m_unk04;//

		long long m_unk05;
		long long m_unk06;//

		long long m_unk07;
		long long m_unk08;

		long long m_unk09;
		long long m_unk10;

		long long m_unk11;
		long long m_unk12;
	};

	struct MeshGroup
	{
		unsigned int m_numFaceGroups;
		unsigned int m_lodIndex;
		unsigned long long m_unk00;

		unsigned long long m_offsetVertexBuffer;
		unsigned long long m_unk01;

		unsigned long long m_unk02;
		unsigned long long m_unk03;

		unsigned long long m_offsetFVFInfo;
		unsigned long long m_numVertices;

		unsigned long long m_unk04;
		unsigned long long m_unk05;
	};

	struct LodInfo
	{
		float m_unk00[12];
	};

	struct MeshHeader
	{
		unsigned int m_magic;
		unsigned int m_fileVersion;
		unsigned int m_fileSize;
		unsigned int m_totalNumTris;//Of all face groups in mesh * 3

		float m_boundingBoxCenter[4];
		float m_boundingBoxMin[4];
		float m_boundingBoxMax[4];
		float m_unk00[3];//Maybe scale? bounding sphere? last 32bits could be primitive draw type see Lara hair splines
		unsigned int m_primitiveDrawType;
		unsigned int m_padding[8];//Always 0?

		unsigned long long m_unk01;//maybe flags?
		unsigned long long m_offsetFaceGroups;

		unsigned long long m_offsetMeshGroups;
		unsigned long long m_offsetBoneIndexList;

		unsigned long long m_offsetLodInfo;
		unsigned long long m_offsetFaceBuffer;

		unsigned short m_numFaceGroups;
		unsigned short m_numMeshGroups;
		unsigned short m_numBones;
		unsigned short m_numLods;//see deer.drm
	};

#pragma pack(pop)

	enum kPrimitiveDrawType
	{
		LINE_LIST,
		TRIANGLE_LIST
	};


	struct Model
	{
		TRDE::MeshHeader m_meshHeader;
		std::vector<char*> m_vertexBuffers;
		std::vector<TRDE::MeshGroup> m_meshGroups;
		std::vector<std::vector<TRDE::VertexDeclaration>> m_vertexDeclarations;
		std::vector<TRDE::VertexDeclarationHeader> m_vertexDeclarationHeaders;
		std::vector<TRDE::FaceGroup> m_faceGroups;
		std::vector<char*> m_indexBuffers;
	};

	struct Bone
	{
		float m_pos[3];
	};

	struct Skeleton
	{
		std::vector<Bone> m_bones;
	};

	void loadModel(TRDE::Model& model, std::ifstream& stream);
	void loadSkeleton(TRDE::Skeleton& skeleton);
	std::vector<TRDE::VertexDeclaration> ReorderVertDecls(std::vector<TRDE::VertexDeclaration>& vertDecl, TRDE::MeshGroup& currentMeshGroup, char* vertexBuffer, TRDE::VertexDeclarationHeader& vertDeclHeader);
	void destroyModel(TRDE::Model& model);
};