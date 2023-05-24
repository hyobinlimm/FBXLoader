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
		Vector3 position = {};			// 좌표상의 위치값

		Vector2 uv = {};

		Vector3 normal = {};		// 노말값

		Vector3 tangent = {};		// data 없음. 

		Vector3 bitangent = {};		// data 없음.
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
		float emissive[4];	// 발광색

		float roughness;
		float shininess;	// 빛남의 광도
		float transparency; // 투명도 
		float reflectivity; // 반사율

		tstring baseColorFile;		// baseColor + opacity
		tstring normalMapFile;
		tstring roughnessMapFile;	// roughness + metallic + AO 
		tstring emissiveFile;
		tstring ambientFile;
		tstring specularFile;
	};

	struct FBXNodeInfo
	{
		// 이 노드의 이름
		tstring nodeName;
		// 부모 노드의 이름
		tstring parentNodeName;

		// Mesh Node 여부
		bool hasMesh;

		// Nagative scale 여부
		bool isNegative;

		// MeshNode의 이름
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
		std::wstring typeName; // 사용 할지 안할지 모름

		std::vector<keyFrameInfo> keyFrameInfo;

		float frameRate; // 엔진에서 60프레임으로 돌린다고 했을때, 한 프레임을 얼만큼 보여줄건지에 대한 비율
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
