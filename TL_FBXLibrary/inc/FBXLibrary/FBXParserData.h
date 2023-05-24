#pragma once

#include <vector>
#include "Common/Common_typedef.h"
#include "Math/TL_Math.h"

namespace TL_FBXLibrary
{
	using namespace TL_Math;

	struct MyFace
	{
		UINT index[3] = {};
	};

	struct StaticVertex
	{
		Vector3 pos = {};			// 좌표상의 위치값

		Vector2 uv = {};
		 
		Vector3 normal = {};		// 노말값

		Vector3 tangent = {};		// 노말값

		Vector3 bitangent = {};		// 노말값
	};

	struct SkeletalVertex
	{
		Vector3 pos = {};

		Vector3 normal = {};

		Vector2 uv = {};

		Vector3 tangent = {};		// data 없음. 

		Vector3 bitangent = {};		// data 없음.

		int index[4] = {};

		Vector4 weight1 = {};
		//Vector4 weight2 = {};
	};

	struct BoneWeight
	{
		int index[4] = { -1, -1, -1, -1 };
		Vector4 weight1 = {};
		// Vector4 weight2 = {};
	};

	struct MeshInfo
	{
		tstring meshNodeName;

		std::vector<unsigned int> materialIndexs;

		bool isMaterial = false; 
		bool isStatic = true;  
		bool isAnimation =  false;

		std::vector<StaticVertex> StaticVertex;
		std::vector<SkeletalVertex> SkeletalVertex;

		std::vector<std::pair<std::vector<MyFace>, uint>> indexBuffer;

	};

	struct MySkeleton
	{
		tstring boneName;
		tstring parentboneName; 

		int parentBoneIndex;
		
		/**
		 * @brief 3d scene space에서 정의된 bone의 transform입니다.
		 * 흔히 nodeTM이라고도 부릅니다.
		 *
		*/
		TL_Math::Matrix m_WorldTM;
		
		/**
		 * @brief 3d scene space에서 정의된 vertex를 bone space로 변환하는 행렬입니다.
		 * 이는 bone의 transform의 역변환, 그러니까 즉 bone의 nodeTM의 역행렬과 매우 유사하거나 일치한 변환입니다.
		 * 스키닝 메시의 모든 vertex들의 위치를 결정할 때, 이 행렬에 bone 게임 오브젝트의 현재 worldTM을 곱해 bone들의 worldTM을 구하고,
		 * vertex가 영향을 받는 bone의 가중치만큼 이 bone들의 worldTM에 영향을 받아 vertex의 월드상 위치를 결정하게 됩니다.
		*/
		TL_Math::Matrix m_bone_From3DSceneSpaceToBoneSpaceTM;
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

		tstring baseColorFile; // baseColor + opacity
		tstring normalMapFile;
		tstring roughnessMapFile; // roughness + metallic + AO 
		tstring emissiveFile;
		tstring ambientFile;
		tstring specularFile;
	};

	struct FBXNodeInfo
	{
		tstring nodeName;		// 이 노드의 이름
		tstring parentNodeName;	// 부모 노드의 이름
		tstring meshNodeName;	// MeshNode의 이름

		bool hasMesh;			// Mesh Node 여부

		bool isNegative;		// Nagative scale 여부

		MySkeleton SkeletonInfo;

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
		tstring boneName;

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
		tstring name; // Scene 이름

		std::vector<TL_FBXLibrary::FBXMaterialInfo> m_MaterialList;
		std::vector<TL_FBXLibrary::MeshInfo> m_MeshList;
		std::vector<TL_FBXLibrary::FBXNodeInfo> m_ObjNodeList;
		std::vector<TL_FBXLibrary::MySkeleton> m_BoneList;
		std::vector<TL_FBXLibrary::Animation> m_AnimList;
	};
}
