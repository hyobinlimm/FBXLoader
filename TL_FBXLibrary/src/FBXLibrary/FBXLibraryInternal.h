#pragma once

#include "fbxsdk.h"
#include <Common.h>
#include "Math/TL_Math.h"
#include <vector>
#include "..\Serializable.h"

namespace TL_FBXLibrary
{
	struct FBXPrefab;
	struct MySkeleton;
	struct MeshInfo;
	struct FBXNodeInfo;
	struct FBXMaterialInfo;
	struct BoneWeight;
	struct AnimationInfo; 
	struct KeyFrameInfo;
	struct StaticVertex;
	struct SkeletalVertex;
}

struct FBXNodePointer
{
	FbxNode* m_pFBXParentNode;
	FbxNode* m_pFBXMyNode;
};

class Float4X4;

/**
 * \brief DLL로 노출되는 인터페이스 안에 포함되어 있는,
 * 실제로 일을 하는 FBX라이브러리 (안에 SDK를 래핑해서 구현됨)
 *
 * 어쩌고 저쩌고..
 * 2023.01.20 효빈짱
 */
class FBXLibraryInternal
{
public:
	FBXLibraryInternal();
	~FBXLibraryInternal();

public:
	bool Init();
	bool Release();

	void FilesLoad(); // Model 폴더에 있는 모든 파일은 시리얼라이즈 함.

	void FBXSerialization(tstring filename);

	bool Load(tstring filename);

	bool BinaryLoad(const tstring filename);
	bool BinaryFolderLoad(const tstring filename);
	bool PushBinaryData(PrefabData preFab); 
	bool MultipleAnimationBinaryData(PrefabData preFab);

private:
	// Scene 의 정보 해석 
	void LoadAnimation();
	void ProcessNode(FbxNode* pParentNode, FbxNode* pNode); // 루트 노드부터 재귀 함수를 통해 오브젝트 개수와 종류를 알아옴
	void ProcessSkeleton(FbxNode* pParentNode, FbxNode* pNode);
	void ProcessBoneWeights(FbxNode* pParentNode, FbxNode* pNode);
	void ProcessAnimationData(FbxNode* pParentNode, FbxNode* pNode, tstring boneName);

	tstring LoadFBXTextureProperty(FbxProperty& prop);
	int FindBoneIndex(tstring boneName);
	int FindMaterialIndex(tstring materialName);

	// matrix 함수들
	TL_Math::Matrix MakeFBXLocalNodeTM(FbxNode* pNode, bool& negative);
	TL_Math::Matrix CheckNegativeScale(TL_Math::Matrix matrix);
	FbxAMatrix GetTransformMatrix(FbxNode* pNode);
	FbxAMatrix GetQuaternionToFBXMatrix(FbxQuaternion rotation);
	TL_Math::Vector4 ConvertVector4(FbxVector4 v4);
	TL_Math::Matrix ConvertMatrix(FbxMatrix matrix);
	TL_Math::Vector4 Vector4Transfrom(TL_Math::Vector4 v4, TL_Math::Matrix m);

	FbxVector4 ConvertToFbxVector(TL_Math::Vector4 vector){ return FbxVector4{ vector.x, vector.z, vector.y, vector.w }; }
	FbxVector4 ConvertToFbxVector(TL_Math::Vector3 vector){ return FbxVector4{ vector.x, vector.z, vector.y, 1.f }; }
	FbxMatrix ConvertToFbxMatrix(TL_Math::Matrix matrix)
	{
		return FbxMatrix{
		    matrix.m[0][0], matrix.m[0][2], matrix.m[0][1], matrix.m[0][3],
			matrix.m[2][0], matrix.m[2][2], matrix.m[2][1], matrix.m[2][3],
			matrix.m[1][0], matrix.m[1][2], matrix.m[1][1], matrix.m[1][3],
		    matrix.m[3][0], matrix.m[3][2], matrix.m[3][1], matrix.m[3][3]
		};
	}

	Float4X4 ConvertToFloat4X4(TL_Math::Matrix matrix)
	{
		return  Float4X4
		{ matrix._11, matrix._12, matrix._13, matrix._14
			,matrix._21, matrix._22, matrix._23, matrix._24
			,matrix._31, matrix._32, matrix._33, matrix._34
			,matrix._41, matrix._42, matrix._43, matrix._44
		};
	}

public:
	// 최적화
	void AllConvertOptimize();

	// GetMeshInfo
	int GetObjMeshListSize();

private:
	// vertex 최적화에 필요한 함수들
	bool ConvertOptimize(TL_FBXLibrary::MeshInfo& meshInfo, int nodeIndex, FbxMesh* fbxMesh, tstring matName);
	bool NonMaterialConvertOptimize(TL_FBXLibrary::MeshInfo& meshInfo, int nodeIndex, FbxMesh* fbxMesh);

	void Static(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative, tstring matName);
	void Static(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, TL_FBXLibrary::FBXNodeInfo nodeInfo, tstring matName);
	void Skinned(int nodeIndex, TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative, tstring matName);

	void NonMaterialStatic(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative);
	void NonMaterialSkinned(int nodeIndex, TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative);

	// subMesh에 필요한 함수들
	void MeshSplitPerMaterial(int nodeIndex);
	void LoadMaterial(TL_FBXLibrary::MeshInfo& meshInfo, FbxSurfaceMaterial* fbxMaterialData, std::vector<TL_FBXLibrary::FBXMaterialInfo>& matInfo);

	TL_Math::Vector2 GetUV(FbxMesh* fbxMesh, const int faceIndex, const int vertexIndex);
	void PushTangentAndBiTangent(TL_FBXLibrary::MeshInfo& meshInfo);

	void StaticVertexTangent(TL_FBXLibrary::MeshInfo& meshInfo, UINT* face);
	void SkinnedVertexTangent(TL_FBXLibrary::MeshInfo& meshInfo, UINT* face);

public:
	// serialize 에 필요한 함수
	void TextSerialize();
	void BinarySerialize();


private:
	// fbxSdk 초기화 및 import에 필요한 변수들
	// FbxManager가 소멸될 때 Scene과 Importer를 정리하기 때문에 이것만 스마트 포인터 사용.
	std::unique_ptr<FbxManager, void(*)(FbxManager*)> m_pFbxManager;  
	FbxImporter* m_pFbxImporter;
	FbxScene* m_pFbxScene;

public:
	std::unique_ptr<TL_FBXLibrary::FBXPrefab> m_pFBXPrefab;
	TL_FBXLibrary::FBXPrefab* m_bineryFrefab; // test용 임시변수

private:

	int m_meshCount; // boneWeight값 저장할 때 필요

	FbxArray<FbxString*> m_AnimNames;

	std::vector<std::unique_ptr<FBXNodePointer>> m_FBXNodeHierarchy;
	std::vector<std::vector<TL_FBXLibrary::BoneWeight>> m_BoneWeights; // mesh마다 들고 있는 bone를 관리하기 위해 2중벡터 사용
};

inline FbxAMatrix FBXLibraryInternal::GetTransformMatrix(FbxNode* pNode)
{
	const FbxVector4 translation = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 rotation = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 scaling = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(translation, rotation, scaling);
}

inline FbxAMatrix FBXLibraryInternal::GetQuaternionToFBXMatrix(FbxQuaternion rotation)
{
	const FbxVector4 _temp = { 0.0f, 0.0f , 0.0f , 1.f };

	FbxAMatrix R = { _temp, rotation, _temp };

	return R;
}

inline TL_Math::Vector4 FBXLibraryInternal::ConvertVector4(FbxVector4 v4)
{
	// xyzw -> xzyw
	return TL_Math::Vector4
	(
		static_cast<float>(v4.mData[0]),
		static_cast<float>(v4.mData[2]),
		static_cast<float>(v4.mData[1]),
		static_cast<float>(v4.mData[3])
	);
}

inline TL_Math::Matrix FBXLibraryInternal::ConvertMatrix(FbxMatrix matrix)
{
	const FbxVector4 _r1 = matrix.GetRow(0);
	const FbxVector4 _r2 = matrix.GetRow(1);
	const FbxVector4 _r3 = matrix.GetRow(2);
	const FbxVector4 _r4 = matrix.GetRow(3);

	return TL_Math::Matrix
	(
		ConvertVector4(_r1),
		ConvertVector4(_r3),
		ConvertVector4(_r2),
		ConvertVector4(_r4)
	);
}

inline TL_Math::Vector4 FBXLibraryInternal::Vector4Transfrom(TL_Math::Vector4 v4, TL_Math::Matrix m)
{
	TL_Math::Vector4 _vector4 = {
		 (v4.x * m._11) + (v4.y * m._12) + (v4.z * m._13) + (v4.w * m._14)
		,(v4.x * m._21) + (v4.y * m._22) + (v4.z * m._23) + (v4.w * m._24)
		,(v4.x * m._31) + (v4.y * m._32) + (v4.z * m._33) + (v4.w * m._34)
		,(v4.x * m._41) + (v4.y * m._42) + (v4.z * m._43) + (v4.w * m._44)
	};

	return _vector4;
}
