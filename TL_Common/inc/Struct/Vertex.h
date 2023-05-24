#pragma once
#include "Math/TL_Math.h"

namespace TL_Common
{
	using namespace TL_Math;

	struct HVFace
	{
		int index[3];
	};

    // 78 bytes
    //struct HVVertex
    //{
    //    TL_Math::Vector3 position;
    //    TL_Math::Vector2 uv;
    //    TL_Math::Vector3 normal;
    //    TL_Math::Vector3 tangent;
    //    TL_Math::Vector3 bitangent;
    //};
	//
    //// 110 bytes (78 bytes + 32 bytes)
    //struct SkeletalMeshVertex :
    //    public HVVertex
    //{
    //    int boneIndex[4];
    //    float boneWeight[4];
    //};

	struct StaticVertex
	{
		Vector3 position = {};			// ��ǥ���� ��ġ��

		Vector2 uv = {};

		Vector3 normal = {};		// �븻��

		Vector3 tangent = {};		// data ����. 

		Vector3 bitangent = {};		// data ����.
	};

	struct SkeletalVertex
	{
		Vector3 position = {};

		Vector3 normal = {};

		Vector2 uv = {};

		Vector3 tangent = {}; 

		Vector3 bitangent = {};

		int index[4] = {};

		Vector4 weight1 = {};
	};

	struct MeshInfo
	{
		tstring meshNodeName;

		std::vector<uint32> materialIndexs;

		bool isStatic = false;
		bool isAnimation = false;

		std::vector<StaticVertex> StaticVertex;
		std::vector<SkeletalVertex> SkeletalVertex;

		std::vector<HVFace> optimizeFace;
		std::vector<std::pair<std::vector<HVFace>, uint>> indexBuffer;
	};

	struct HVSkeleton
	{
		tstring boneName;
		tstring parentboneName;

		int parentBoneIndex;

		TL_Math::Matrix m_WorldTM;
		TL_Math::Matrix m_bone_OffsetTM;
	};

	struct FBXMaterialInfo
	{
		int materialIndex;

		tstring materialName;

		float ambient[4];
		float diffuse[4];
		float specular[4];
		float emissive[4];	// �߱���

		float roughness;
		float shininess;	// ������ ����
		float transparency; // ���� 
		float reflectivity; // �ݻ���

		tstring baseColorFile;		// baseColor + opacity
		tstring normalMapFile;
		tstring roughnessMapFile;	// roughness + metallic + AO 
		tstring emissiveFile;
		tstring ambientFile;
		tstring specularFile;
	};

	struct FBXNodeInfo
	{
		// �� ����� �̸�
		tstring nodeName;
		// �θ� ����� �̸�
		tstring parentNodeName;

		// Mesh Node ����
		bool hasMesh;

		// Nagative scale ����
		bool isNegative;

		// MeshNode�� �̸�
		tstring meshNodeName;

		HVSkeleton SkeletonInfo;

		TL_Math::Matrix nodeTM;
	};

	struct AnimationInfo
	{
		float time;

		TL_Math::Vector3 pos;
		TL_Math::Quaternion rot;
		TL_Math::Vector3 scl;
	};

	struct keyFrameInfo
	{
		tstring nodeName;
		tstring bonename;

		std::vector<AnimationInfo> animInfo;
	};

	struct Animation
	{
		std::wstring  name;
		std::wstring typeName; // ��� ���� ������ ��

		std::vector<keyFrameInfo> keyFrameInfo;

		float frameRate; // �������� 60���������� �����ٰ� ������, �� �������� ��ŭ �����ٰ����� ���� ����
		int tickPerFrame;
		int totalKeyFrame;
		int startKeyFrame;
		int endKeyFrame;
	};

	struct FBXPrefab
	{
		std::vector<TL_Common::FBXMaterialInfo> m_MaterialList;
		std::vector<TL_Common::MeshInfo> m_MeshList;
		std::vector<TL_Common::FBXNodeInfo> m_pObjMeshList;
		std::vector<TL_Common::HVSkeleton> m_pBoneList;
		std::vector<TL_Common::Animation> m_AnimList;
	};
}
