#pragma once

#include <string>
#include <vector>

namespace TRAS
{
#pragma pack(push, 1)

	struct VertexDeclaration
	{
		unsigned int m_componentNameHashed;
		unsigned short m_position;
		unsigned short m_unk00;
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
		unsigned int m_numVerts;
		unsigned int m_unk01;//flags?

		unsigned int m_unk02;
		unsigned int m_unk03;
		unsigned int m_materialIndex;
		unsigned int m_unk04;

		unsigned int m_unk05;
		unsigned int m_unk06;
		unsigned int m_materialIndex2;
		int m_unk07[5];//Always -1

	};

	struct MeshGroup
	{
		unsigned int m_numFaceGroups;
		unsigned short m_modelIndex;
		unsigned short m_numBoneMapIndices;
		unsigned int m_offsetBoneMap;
		unsigned int m_offsetVertexBuffer;

		unsigned int m_padding[3];//Always 0?
		unsigned int m_offsetFVFInfo;

		unsigned int m_numVertices;
		unsigned int m_unk00;//Always 0
		unsigned int m_sumOfAllPreviousFaceGroupTris;//Number of all tris, of all face groups read so far (all prior to the current one).
		unsigned int m_sumOfAllFaceGroupTris;//Number of tris of all face groups of this mesh group
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
		float m_unk00[3];//Maybe scale? bounding sphere?
		unsigned int m_primitiveDrawType;
		int m_padding[8];//?

		unsigned int m_unk01;//maybe flags?
		unsigned int m_offsetFaceGroups;
		unsigned int m_offsetMeshGroups;
		unsigned int m_offsetBoneIndexList;

		unsigned int m_offsetLodInfo;
		unsigned int m_offsetFaceBuffer;

		unsigned short m_numFaceGroups;
		unsigned short m_numMeshGroups;
		unsigned short m_numBones;
		unsigned short m_numModels;//See deer.drm, each is treated as a "lod"
	};

	struct Bone
	{
		float m_pos[3];
	};

	struct Skeleton
	{
		std::vector<Bone> m_bones;
	};

#pragma pack(pop)

	struct Model
	{
		TRAS::MeshHeader m_meshHeader;
		std::vector<TRAS::MeshGroup> m_meshGroups;
		std::vector<TRAS::FaceGroup> m_faceGroups;
	};

	void loadModel(TRAS::Model& model, std::ifstream& stream);
	void loadSkeleton(TRAS::Skeleton& skeleton);;
};