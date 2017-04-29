#pragma once

#include <string>
#include <vector>

namespace TRDE
{
	enum kMeshType
	{
		TRESS_FX, //Line list
		SKELETAL_MESH,
		STATIC_MESH //No bones
	};

#pragma pack(push, 1)

	struct VertexDeclaration
	{
		unsigned int m_componentNameHashed;
		unsigned short m_position;
		unsigned short m_unk00;//Maybe data type/shader channel, unconfirmed.
	};

	struct VertexDeclarationHeader
	{
		unsigned int m_unk00;//Possibly legacy D3D9 FVF
		unsigned int m_unk01;
		unsigned short m_componentCount;
		unsigned short m_vertexStride;
		unsigned int m_padding;
	};

	struct FaceGroup
	{
		float m_unk00[4];

		unsigned int m_indexBufferStartIndex;
		unsigned int m_numTris;
		unsigned int m_numVerts;
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

	struct VertexGroup
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

		unsigned int m_sumOfAllPreviousFaceGroupTris;//Number of all tris * 3, of all face groups read so far (all prior to the current one).
		unsigned int m_sumOfAllFaceGroupTris;//Number of tris of all face groups of this mesh group
		unsigned long long m_unk04;
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
		enum kMeshType m_meshType;
		unsigned int m_padding[8];//Always 0?

		unsigned long long m_unk01;//maybe flags?
		unsigned long long m_offsetFaceGroups;

		unsigned long long m_offsetVertexGroups;
		unsigned long long m_offsetBoneIndexList;

		unsigned long long m_offsetLodInfo;
		unsigned long long m_offsetFaceBuffer;

		unsigned short m_numFaceGroups;
		unsigned short m_numVertexGroups;
		unsigned short m_numBones;
		unsigned short m_numLods;//see deer.drm
	};

#pragma pack(pop)

	struct Model
	{
		TRDE::MeshHeader m_meshHeader;
		std::vector<char*> m_vertexBuffers;
		std::vector<TRDE::VertexGroup> m_vertexGroups;
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
	std::vector<TRDE::VertexDeclaration> ReorderVertDecls(std::vector<TRDE::VertexDeclaration>& vertDecl, TRDE::VertexGroup& currentVertexGroup, char* vertexBuffer, TRDE::VertexDeclarationHeader& vertDeclHeader);
	void destroyModel(TRDE::Model& model);
};