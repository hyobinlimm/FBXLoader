#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "../Serializable.h"

#include "FBXLibraryInternal.h"
#include "FBXLibrary/FBXParserData.h"
#include <string>
#include <filesystem>
#include <map>

FBXLibraryInternal::FBXLibraryInternal()
	: m_pFbxManager(nullptr, [](FbxManager* ptr) {}),
	m_pFbxImporter(nullptr),
	m_pFbxScene(nullptr),
	m_pFBXPrefab(nullptr),
	m_meshCount(-1)
{
}

FBXLibraryInternal::~FBXLibraryInternal()
{

}

bool FBXLibraryInternal::Init()
{
	///TODO:: 예제 코드에 Commom.h InitializeSdkObjects() 함수를 참고해서 다듬자. 

	// FBX SDK 준비
	m_pFbxManager = std::move(std::unique_ptr<FbxManager, void(*)(FbxManager*)>(FbxManager::Create(),
		[](FbxManager* ptr) {ptr->Destroy(); }));

	///TODO:: FbxIOSettings 클래스가 정확히 하는 일이 뭔지 공부하기.  
	FbxIOSettings* _pFbxIOSettings = FbxIOSettings::Create(m_pFbxManager.get(), IOSROOT);
	m_pFbxManager->SetIOSettings(_pFbxIOSettings);

	// 데이터 확인용 임시 코드
	// int num = TestCode();

	return  true;
}

bool FBXLibraryInternal::Release()
{
	m_AnimNames.Clear();
	delete m_AnimNames;

	m_BoneWeights.clear();

	m_FBXNodeHierarchy.clear();

	// 소멸자가 불릴 때 m_pFbxManager의 소멸자에서 Scene과 Importer를 정리하지만 명시적으로 한번 정리해주는 것. 
	//m_pFbxScene->Destroy();
	//m_pFbxImporter->Destroy();

	return true;
}

void FBXLibraryInternal::FilesLoad()
{
	const std::filesystem::path _path("../../../../../4_Resources/_DevelopmentAssets");

	const bool _isExists = std::filesystem::exists(_path); // 저 경로가 존재하는가? 
	const bool _isDirectory = std::filesystem::is_directory(_path); // 경로인가?
	assert(_isDirectory != false); // 걸렷다면 파일경로가 잘못된 것.  

	std::filesystem::recursive_directory_iterator itr(_path / "Model");

	while (itr != std::filesystem::end(itr))
	{
		const std::filesystem::directory_entry& entry = *itr;

		const tstring _fileName = entry.path().c_str(); // entry에서 받오는 타입은 wstring

		const int _ext = _fileName.rfind(L"fbx"); // .fbx 파일인지 탐색, 파일 경로에 .fbx 이름이 없으면 -1 반환

		if (_ext != -1)
		{
			Load(_fileName);

			AllConvertOptimize();

			BinarySerialize();

			Release();
		}

		++itr;
	}
}

void FBXLibraryInternal::FBXSerialization(tstring filename)
{
	Init();

	Load(filename);

	AllConvertOptimize();

	BinarySerialize();

	Release();
}

bool FBXLibraryInternal::Load(tstring filename)
{
	m_pFbxImporter = FbxImporter::Create(m_pFbxManager.get(), "");

	m_pFbxScene = FbxScene::Create(m_pFbxManager.get(), ""); // 씬에 모든 데이터가 담겨 있음. 

	std::string _fileName = StringHelper::WStringToString(filename);

	bool _bResult = m_pFbxImporter->Initialize(_fileName.c_str(), 0, m_pFbxManager->GetIOSettings());
	assert(_bResult != false);

	_bResult = m_pFbxImporter->Import(m_pFbxScene);

	// fbx에서 단위를 조정해줌.( 암시적으로 센티미터 단위로 변환. )
	//FbxSystemUnit lFbxFileSysteomUnit = m_pFbxScene->GetGlobalSettings().GetSystemUnit();
	//FbxSystemUnit lFbxOriginSystemUnit = m_pFbxScene->GetGlobalSettings().GetOriginalSystemUnit();
	//double factor = lFbxFileSysteomUnit.GetScaleFactor();
	//
	//const FbxSystemUnit::ConversionOptions lConversionOptions =
	//{
	//  true,
	//  true,
	//  true,
	//  true,
	//  true,
	//  true
	//};
	//
	//lFbxFileSysteomUnit.m.ConvertScene(m_pFbxScene, lConversionOptions);
	//lFbxOriginSystemUnit.m.ConvertScene(m_pFbxScene, lConversionOptions);

	// Axis 를 DirectX에 맞게 변환
	//m_pFbxScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::DirectX);

	FbxGeometryConverter* geometryConverter = new FbxGeometryConverter(m_pFbxManager.get());

	geometryConverter->Triangulate(m_pFbxScene, true, true);

	// 하나의 mesh에 여러개의 material를 가지고 있다면, 하나의 mesh가 하나의 material를 가지게 함. 
	geometryConverter->SplitMeshesPerMaterial(m_pFbxScene, true);

	// fbx Scene 의 모든 정보를 들고 있을 구조체 생성. 
	m_pFBXPrefab = std::make_unique<TL_FBXLibrary::FBXPrefab>();
	m_pFBXPrefab->name = StringHelper::StringToWString(_fileName);

	if (_bResult)
	{
		// FBX는 트리구조
		// 재귀호출로 순회 가능, N 트리여서 자식 수를 알아야 함.
		FbxNode* _pRootNode = m_pFbxScene->GetRootNode();

		LoadAnimation();

		// 씬에 저장 된 트리구조를 순회해서 오브젝트를 찾아옴.
		ProcessNode(nullptr, _pRootNode);
		ProcessSkeleton(nullptr, _pRootNode);
		ProcessBoneWeights(nullptr, _pRootNode);

		m_meshCount = -1; // 다 조사했으면 다시 리셋
	}

	return _bResult;
}

bool FBXLibraryInternal::BinaryLoad(tstring filename)
{
	// create and open an archive for input
	std::ifstream ifs(filename, std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);

	PrefabData _prefab;
	ia& _prefab;

	bool _result = PushBinaryData(_prefab);

	return _result;
}

bool FBXLibraryInternal::BinaryFolderLoad(const tstring filename)
{
	bool _result = false; // 결과 반환 변수.

	const std::filesystem::path _path(filename);

	const bool _isExists = std::filesystem::exists(_path); // 저 경로가 존재하는가?
	const bool _isDirectory = std::filesystem::is_directory(_path); // 경로인가?
	assert(_isDirectory != false); // 걸렷다면 파일경로가 잘못된 것.  

	std::filesystem::directory_iterator itr(_path);

	m_pFBXPrefab = std::make_unique<TL_FBXLibrary::FBXPrefab>(); // 이 폴더의 .fbx의 Mesh는 동일, animation만 추가로 넣어주자. 

	while (itr != std::filesystem::end(itr))
	{
		const std::filesystem::directory_entry& entry = *itr;

		const tstring _fileName = entry.path().c_str(); // entry에서 받오는 타입은 wstring

		const int _ext = _fileName.rfind(L"TL"); // .fbx 파일인지 탐색, 파일 경로에 .fbx 이름이 없으면 -1 반환

		if (_ext != -1)
		{
			// create and open an archive for input
			std::ifstream ifs(_fileName, std::ios::binary);
			boost::archive::binary_iarchive ia(ifs);

			PrefabData _prefab;
			ia& _prefab;

			_result = MultipleAnimationBinaryData(_prefab);

		}

		++itr;
	}

	return _result;
}

void FBXLibraryInternal::LoadAnimation()
{
	m_pFbxScene->FillAnimStackNameArray(OUT m_AnimNames);

	for (int i = 0; i < m_AnimNames.GetCount(); i++)
	{
		const FbxAnimStack* _pAnimStack = m_pFbxScene->FindMember<FbxAnimStack>(m_AnimNames[i]->Buffer());

		if (_pAnimStack == nullptr) continue;

		const FbxTakeInfo* _takeInfo = m_pFbxScene->GetTakeInfo(_pAnimStack->GetName());

		const double _startTime = _takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
		const double _endTime = _takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
		const double _frameRate = static_cast<float>(FbxTime::GetFrameRate(m_pFbxScene->GetGlobalSettings().GetTimeMode())); // timeMode와 관련된 프레임 속도를 초당 프레임 단위로 가져옴.

		TL_FBXLibrary::Animation _animInfo;

		_animInfo.name = StringHelper::StringToWString(_pAnimStack->GetName());

		_animInfo.frameRate = static_cast<float>(_frameRate);

		if (_startTime < _endTime)
		{
			_animInfo.totalKeyFrame = static_cast<int>((_endTime - _startTime) * static_cast<double>(_frameRate));
			_animInfo.endKeyFrame = static_cast<int>((_endTime - _startTime) * static_cast<double>(_frameRate));
			_animInfo.startKeyFrame = static_cast<int>(_startTime) * _animInfo.totalKeyFrame;
			_animInfo.tickPerFrame = static_cast<int>((_endTime - _startTime) / _animInfo.totalKeyFrame);
		}

		m_pFBXPrefab->m_AnimList.push_back(_animInfo);
	}
}

tstring FBXLibraryInternal::LoadFBXTextureProperty(FbxProperty& prop)
{
	tstring name = TEXT("");

	if (!prop.IsValid()) // 값이 없으면
		return name;

	const int count = prop.GetSrcObjectCount();
	if (count == 0) // 값이 없다면
		return name;

	FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>(0);
	assert(texture != nullptr); // 값이 있는데 텍스처가 존재하지 않으면 오류

	const std::string _fileName = std::filesystem::path(texture->GetRelativeFileName()).filename().string();
	name = StringHelper::ToTString(_fileName);

	return name;
}

void FBXLibraryInternal::ProcessNode(FbxNode* pParentNode, FbxNode* pNode)
{
	// mesh 가 아니라면 return
	if (pNode->GetCamera() || pNode->GetLight()) return;

	FbxMesh* _pMesh = pNode->GetMesh();

	// FBX Node 가 트리구조에서 가져와 저장해 놓자. 
	FBXNodePointer* _pNode = new FBXNodePointer();

	_pNode->m_pFBXParentNode = pParentNode;
	_pNode->m_pFBXMyNode = pNode;

	m_FBXNodeHierarchy.emplace_back(_pNode);

	// 미리 저장 공간 생성. 
	TL_FBXLibrary::FBXNodeInfo _pNodeInfo;

	_pNodeInfo.isNegative = false;
	_pNodeInfo.nodeName = StringHelper::StringToWString(pNode->GetName());
	_pNodeInfo.nodeTM = MakeFBXLocalNodeTM(pNode, _pNodeInfo.isNegative);

	if (pParentNode != nullptr)
		_pNodeInfo.parentNodeName = StringHelper::StringToWString(pParentNode->GetName());

	_pNodeInfo.hasMesh = false;
	if (pNode->GetMesh())
	{
		_pNodeInfo.hasMesh = true;
		_pNodeInfo.meshNodeName = StringHelper::StringToWString(pNode->GetMesh()->GetName());
	}

	m_pFBXPrefab->m_ObjNodeList.push_back(_pNodeInfo);


	const unsigned int _numChildNode = pNode->GetChildCount();

	for (unsigned int i = 0; i < _numChildNode; ++i)
	{
		FbxNode* _child = pNode->GetChild(i);
		ProcessNode(pNode, _child);
	}
}

void FBXLibraryInternal::ProcessSkeleton(FbxNode* pParentNode, FbxNode* pNode)
{
	// 뼈를 가져온다. 
	if (pNode->GetSkeleton())
	{
		// FBX Node 가 트리구조에서 가져와 저장해 놓자. 
		FBXNodePointer* _pNode = new FBXNodePointer();
		_pNode->m_pFBXParentNode = pParentNode;
		_pNode->m_pFBXMyNode = pNode;
		m_FBXNodeHierarchy.emplace_back(_pNode);

		// 뼈에 대한 정보 저장. 
		TL_FBXLibrary::FBXNodeInfo _fbx;

		_fbx.SkeletonInfo.parentBoneIndex = -1;

		if (pParentNode != nullptr)
		{
			const tstring _parentName = StringHelper::StringToWString(pParentNode->GetName());

			_fbx.SkeletonInfo.parentboneName = _parentName;
			_fbx.SkeletonInfo.parentBoneIndex = FindBoneIndex(_parentName);
		}

		_fbx.SkeletonInfo.boneName = StringHelper::StringToWString(pNode->GetName());

		m_pFBXPrefab->m_BoneList.push_back(_fbx.SkeletonInfo);

		/// Get bone animation data
		ProcessAnimationData(pParentNode, pNode, _fbx.SkeletonInfo.boneName);
	}

	const int _numChildNode = pNode->GetChildCount();

	for (int i = 0; i < _numChildNode; ++i)
	{
		FbxNode* _child = pNode->GetChild(i);
		ProcessSkeleton(pNode, _child);
	}
}

void FBXLibraryInternal::ProcessBoneWeights(FbxNode* pParentNode, FbxNode* pNode)
{
	if (pNode->GetMesh())
	{
		m_meshCount++;

		// mesh 가 들고 있는 vertex 의 수만큼 버퍼공간 생성. 
		int _vertexSize = pNode->GetMesh()->GetControlPointsCount();
		std::vector<TL_FBXLibrary::BoneWeight> _temp(_vertexSize);
		m_BoneWeights.push_back(_temp);

		const int _deformerCount = pNode->GetMesh()->GetDeformerCount();

		// deformer 에서 skin 의 정보를 가져오자. 
		for (int deformerIndex = 0; deformerIndex < _deformerCount; ++deformerIndex)
		{
			FbxSkin* _pSkin = reinterpret_cast<FbxSkin*>(pNode->GetMesh()->GetDeformer(deformerIndex, FbxDeformer::eSkin));

			if (_pSkin == nullptr) continue;

			if (_pSkin->GetSkinningType() == FbxSkin::eRigid)
			{
				// cluster 가 들고 있는 weight 의 값을 넣어주자. 
				const int _clusterCount = _pSkin->GetClusterCount();
				for (int clusterIndex = 0; clusterIndex < _clusterCount; clusterIndex++)
				{
					FbxCluster* _pCluster = _pSkin->GetCluster(clusterIndex);

					if (_pCluster->GetLink() == nullptr) continue;

					const tstring _boneName = StringHelper::StringToWString(_pCluster->GetLink()->GetName());

					const int _boneIndex = FindBoneIndex(_boneName);

					int* _indices = _pCluster->GetControlPointIndices(); // vertex 의 배열 받기
					double* _wegihts = _pCluster->GetControlPointWeights();	// weight 의 배열 받기 

					const int _count = _pCluster->GetControlPointIndicesCount();

					for (int i = 0; i < _count; ++i)
					{
						for (int j = 0; j < 4; ++j)
						{
							if (m_BoneWeights.at(m_meshCount).at(_indices[i]).index[j] == -1)
							{
								m_BoneWeights.at(m_meshCount).at(_indices[i]).index[j] = _boneIndex;

								if (j == 0) m_BoneWeights.at(m_meshCount).at(_indices[i]).weight1.x = static_cast<float>(_wegihts[i]);
								if (j == 1) m_BoneWeights.at(m_meshCount).at(_indices[i]).weight1.y = static_cast<float>(_wegihts[i]);
								if (j == 2) m_BoneWeights.at(m_meshCount).at(_indices[i]).weight1.z = static_cast<float>(_wegihts[i]);
								if (j == 3) m_BoneWeights.at(m_meshCount).at(_indices[i]).weight1.w = static_cast<float>(_wegihts[i]);

								break;
							}
						}
					}

					// worldTM과 offsetTM을 구합니다.
					FbxAMatrix _rootBoneTM;
					FbxAMatrix _boneTM;
					_pCluster->GetTransformMatrix(_rootBoneTM);
					_pCluster->GetTransformLinkMatrix(_boneTM);

					// GeometryTM을 곱하는 부분이 누락되었습니다.
					// 구체적으로 GeometryTM이 어떤 변환인지 모르기 때문에 적지 않았습니다.
					FbxAMatrix _from3DSceneSpaceToBoneSpaceTM = _boneTM.Inverse() * _rootBoneTM;
					//
					//TL_Math::Matrix _offsetTM = ConvertMatrix(_from3DSceneSpaceToBoneSpaceTM);
					//TL_Math::Matrix _nodeTM = ConvertMatrix(_boneTM);

					//const auto _pitch = 90.0f * DirectX::XM_PI / 180.0f;
					//
					////TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromAxisAngle(TL_Math::Vector3::Right, 90.0f * TL_Math::DEG_TO_RAD);
					//TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromYawPitchRoll(0.f, -_pitch, 0.f);
					//
					//_nodeTM *= TL_Math::Matrix::CreateFromQuaternion(_q);
					//
					//m_pFBXPrefab->m_BoneList[_boneIndex].m_WorldTM = _nodeTM;
					//m_pFBXPrefab->m_BoneList[_boneIndex].m_bone_From3DSceneSpaceToBoneSpaceTM = _offsetTM;

					m_pFBXPrefab->m_BoneList[_boneIndex].m_bone_From3DSceneSpaceToBoneSpaceTM = ConvertMatrix(_from3DSceneSpaceToBoneSpaceTM);
					m_pFBXPrefab->m_BoneList[_boneIndex].m_WorldTM = ConvertMatrix(_boneTM);
				}
			}
		}
	}

	const int _numChildNode = pNode->GetChildCount();

	for (int i = 0; i < _numChildNode; ++i)
	{
		FbxNode* _child = pNode->GetChild(i);
		ProcessBoneWeights(pNode, _child);
	}
}

void FBXLibraryInternal::ProcessAnimationData(FbxNode* pParentNode, FbxNode* pNode, tstring boneName)
{
	// 기본적인 정보를 먼저 넣어주자. 
	TL_FBXLibrary::keyFrameInfo _keyFrameInfo;

	_keyFrameInfo.nodeName = StringHelper::StringToWString(pNode->GetName());
	_keyFrameInfo.boneName = boneName;

	// animation 종류를 돌면서 Keyframe를 받아오자. 
	const size_t _animCount = m_pFBXPrefab->m_AnimList.size();

	for (int i = 0; i < _animCount; i++)
	{
		m_pFBXPrefab->m_AnimList[i].keyFrameInfo.push_back(_keyFrameInfo);

		FbxTime::EMode _timeMode = m_pFbxScene->GetGlobalSettings().GetTimeMode();

		FbxAnimStack* _animStack = m_pFbxScene->FindMember<FbxAnimStack>(m_AnimNames[i]->Buffer());
		m_pFbxScene->SetCurrentAnimationStack(OUT _animStack);

		for (FbxLongLong _frame = 0; _frame < m_pFBXPrefab->m_AnimList[i].totalKeyFrame; _frame++)
		{
			FbxTime _fbxTime = 0;
			_fbxTime.SetFrame(_frame, _timeMode);

			FbxAMatrix _localTransform = pNode->EvaluateGlobalTransform(_fbxTime);
			TL_Math::Matrix _localTM;

			if (pParentNode != nullptr)
			{
				// 부모가 skeletal Animation 인지 검사. 
				FbxNodeAttribute* _pParentAttribute = pParentNode->GetNodeAttribute();

				if (_pParentAttribute && _pParentAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					FbxAMatrix _parentWorldTM = pParentNode->EvaluateGlobalTransform(_fbxTime);

					// Local Transform = 부모 Bone의 worldTM의 inverse Transform * 자신 Bone의 worldTM
					_localTransform = _parentWorldTM.Inverse() * _localTransform;
					_localTM = ConvertMatrix(_localTransform);
				}
				else // 부모가 없다면 자기 자신 localTM를 넣어주자. 
				{
					_localTM = ConvertMatrix(_localTransform);

					//const auto _pitch = 90.0f * DirectX::XM_PI / 180.0f;
					//
					////TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromAxisAngle(TL_Math::Vector3::Right, 90.0f * TL_Math::DEG_TO_RAD);
					//TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromYawPitchRoll(0.0f, _pitch, 0.0f);
					//
					//_localTM *= TL_Math::Matrix::CreateFromQuaternion(_q);
				}
			}

			DirectX::XMVECTOR _localPos;
			DirectX::XMVECTOR _localRot;
			DirectX::XMVECTOR _localScl;

			DirectX::XMMatrixDecompose(&_localScl, &_localRot, &_localPos, _localTM);

			TL_FBXLibrary::AnimationInfo _animInfo;

			_animInfo.time = static_cast<float>(_fbxTime.GetSecondDouble());

			_animInfo.pos = TL_Math::Vector3(_localPos);
			_animInfo.rot = TL_Math::Quaternion(_localRot);
			_animInfo.scl = TL_Math::Vector3(_localScl);

			size_t _index = m_pFBXPrefab->m_AnimList[i].keyFrameInfo.size() - 1;
			m_pFBXPrefab->m_AnimList[i].keyFrameInfo[_index].animInfo.push_back(_animInfo);
		}
	}
}

int FBXLibraryInternal::FindBoneIndex(tstring boneName)
{
	if (m_pFBXPrefab->m_BoneList.size() == 0) return -1;

	for (int i = 0; i < m_pFBXPrefab->m_BoneList.size(); ++i)
	{
		if (m_pFBXPrefab->m_BoneList[i].boneName == boneName)
		{
			return i;
		}
	}

	return -1;
}

int FBXLibraryInternal::FindMaterialIndex(tstring materialName)
{
	for (auto mesh : m_pFBXPrefab->m_MaterialList)
	{
		if (mesh.materialName == materialName)
		{
			return mesh.materialIndex;
		}
	}
	return -1;
}

TL_Math::Vector2 FBXLibraryInternal::GetUV(FbxMesh* fbxMesh, const int faceIndex, const int vertexIndex)
{
	TL_Math::Vector2 _uv;

	const int _uvIndex = fbxMesh->GetTextureUVIndex(faceIndex, vertexIndex); // vertex의 uv 가져옴.

	_uv.x = static_cast<float>(fbxMesh->GetElementUV()->GetDirectArray().GetAt(_uvIndex).mData[0]);
	_uv.y = 1.f - static_cast<float>(fbxMesh->GetElementUV()->GetDirectArray().GetAt(_uvIndex).mData[1]);

	return _uv;
}

void FBXLibraryInternal::PushTangentAndBiTangent(TL_FBXLibrary::MeshInfo& meshInfo)
{
	for (int _index = 0; _index < meshInfo.indexBuffer.size(); ++_index)
	{
		for (int _faceIndex = 0; _faceIndex < meshInfo.indexBuffer[_index].first.size(); ++_faceIndex)
		{

			const auto _face = meshInfo.indexBuffer[_index].first[_faceIndex].index;

			if (meshInfo.isStatic == true)
			{
				StaticVertexTangent(meshInfo, _face);
			}
			else
			{
				SkinnedVertexTangent(meshInfo, _face);
			}
		}
	}

	if (meshInfo.isStatic == true)
	{
		for (int i = 0; i < meshInfo.StaticVertex.size(); ++i)
		{
			meshInfo.StaticVertex[i].tangent = TL_Math::XMVector3Normalize(meshInfo.StaticVertex[i].tangent);
			meshInfo.StaticVertex[i].bitangent = TL_Math::XMVector3Normalize(meshInfo.StaticVertex[i].bitangent);

			auto _tangent = meshInfo.StaticVertex[i].tangent;
			// Bumped Normal : normal과 tangent가 정규직교가 아닐 수 있기 때문에 한번 더 계산해주자.
			meshInfo.StaticVertex[i].tangent = _tangent - (_tangent.Dot(meshInfo.StaticVertex[i].normal) * meshInfo.StaticVertex[i].normal);

		}
	}
	else
	{
		for (int i = 0; i < meshInfo.SkeletalVertex.size(); ++i)
		{
			meshInfo.SkeletalVertex[i].tangent = TL_Math::XMVector3Normalize(meshInfo.SkeletalVertex[i].tangent);
			meshInfo.SkeletalVertex[i].bitangent = TL_Math::XMVector3Normalize(meshInfo.SkeletalVertex[i].bitangent);

			auto _tangent = meshInfo.SkeletalVertex[i].tangent;
			meshInfo.SkeletalVertex[i].tangent = _tangent - (_tangent.Dot(meshInfo.SkeletalVertex[i].normal) * meshInfo.SkeletalVertex[i].normal);
		}
	}
}

void FBXLibraryInternal::StaticVertexTangent(TL_FBXLibrary::MeshInfo& meshInfo, UINT* face)
{
	// 삼차원에 존재하는 vertex를 기준으로 방향벡터를 구하자.
	TL_Math::Vector3 e0 = meshInfo.StaticVertex[face[1]].pos - meshInfo.StaticVertex[face[0]].pos;
	TL_Math::Vector3 e1 = meshInfo.StaticVertex[face[2]].pos - meshInfo.StaticVertex[face[0]].pos;

	TL_Math::Vector2 v0 = meshInfo.StaticVertex[face[1]].uv - meshInfo.StaticVertex[face[0]].uv;
	TL_Math::Vector2 v1 = meshInfo.StaticVertex[face[2]].uv - meshInfo.StaticVertex[face[0]].uv;

	float _det = (v0.x * v1.y) - (v0.y * v1.x);

	if (_det == 0)
	{
		_det = 0.0001f; // 0이면 안되니까 보정하려고.  
	}
	const float _r = 1.0f / _det;

	const TL_Math::Vector3 _tangent = ((e0 * v1.y) - (e1 * v0.y)) * _r;
	const TL_Math::Vector3 _biTangent = ((-e0 * v1.x) + (e1 * v0.x)) * _r;

	meshInfo.StaticVertex[face[0]].tangent += _tangent;
	meshInfo.StaticVertex[face[1]].tangent += _tangent;
	meshInfo.StaticVertex[face[2]].tangent += _tangent;

	meshInfo.StaticVertex[face[0]].bitangent += _biTangent;
	meshInfo.StaticVertex[face[1]].bitangent += _biTangent;
	meshInfo.StaticVertex[face[2]].bitangent += _biTangent;
}

void FBXLibraryInternal::SkinnedVertexTangent(TL_FBXLibrary::MeshInfo& meshInfo, UINT* face)
{
	// 현재의 vertex를 기준으로 방향벡터를 구하자.
	TL_Math::Vector3 e0 = meshInfo.SkeletalVertex[face[1]].pos - meshInfo.SkeletalVertex[face[0]].pos;
	TL_Math::Vector3 e1 = meshInfo.SkeletalVertex[face[2]].pos - meshInfo.SkeletalVertex[face[0]].pos;

	TL_Math::Vector2 v0 = meshInfo.SkeletalVertex[face[1]].uv - meshInfo.SkeletalVertex[face[0]].uv;
	TL_Math::Vector2 v1 = meshInfo.SkeletalVertex[face[2]].uv - meshInfo.SkeletalVertex[face[0]].uv;

	float _det = (v0.x * v1.y) - (v0.y * v1.x);
	if (_det == 0)
	{
		_det = 0.0001f; // 0이면 안되니까 보정하려고.  
	}

	const float _r = 1.0f / _det;

	const TL_Math::Vector3 _tangent = ((e0 * v1.y) - (e1 * v0.y)) * _r;
	const TL_Math::Vector3 _biTangent = ((-e0 * v1.x) + (e1 * v0.x)) * _r;

	meshInfo.SkeletalVertex[face[0]].tangent += _tangent;
	meshInfo.SkeletalVertex[face[1]].tangent += _tangent;
	meshInfo.SkeletalVertex[face[2]].tangent += _tangent;

	meshInfo.SkeletalVertex[face[0]].bitangent += _biTangent;
	meshInfo.SkeletalVertex[face[1]].bitangent += _biTangent;
	meshInfo.SkeletalVertex[face[2]].bitangent += _biTangent;
}

TL_Math::Matrix FBXLibraryInternal::MakeFBXLocalNodeTM(FbxNode* pNode, bool& negative)
{
	FbxAMatrix _nodeTrans = m_pFbxScene->GetAnimationEvaluator()->GetNodeLocalTransform(pNode);

	// x 축을 기준으로 회전된 matrix를 게임 엔진에서 쓸 좌표계(xyz -> xzy)로 변환. 
	TL_Math::Matrix _nodeTM = ConvertMatrix(_nodeTrans);

	float _det = _nodeTM.Determinant(); // XMMatrixDeterminant(nodeMatrix);

	if (_det < 0)
	{
		negative = true;

		_nodeTM = CheckNegativeScale(_nodeTM);
	}

	//const auto _roll = 90.0f * DirectX::XM_PI / 180.0f;

	const auto _pitch = 180.0f * DirectX::XM_PI / 180.0f;

	//TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromYawPitchRoll(0.0f, -_roll, 0.0f);

	TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromAxisAngle(TL_Math::Vector3::Right, -(DirectX::XM_PI / 2.0f));

	_nodeTM *= TL_Math::Matrix::CreateFromQuaternion(_q);

	return _nodeTM;
}

TL_Math::Matrix FBXLibraryInternal::CheckNegativeScale(TL_Math::Matrix matrix)
{
	TL_Math::Matrix _nodeTM;

	TL_Math::Vector3 _pos;
	TL_Math::Quaternion _rot;
	TL_Math::Vector3 _scl;

	matrix.Decompose(_scl, _rot, _pos);

	_scl = -1 * _scl;

	TL_Math::Matrix _transformTM = TL_Math::Matrix::CreateTranslation(_pos);
	TL_Math::Matrix _rotationTM = TL_Math::Matrix::CreateFromQuaternion(_rot);
	TL_Math::Matrix _scaleTM = TL_Math::Matrix::CreateScale(_scl);

	_nodeTM = _scaleTM * _rotationTM * _transformTM;

	return  _nodeTM;
}

void FBXLibraryInternal::AllConvertOptimize()
{
	if (m_pFBXPrefab->m_ObjNodeList.size() == 0) return;

	for (int i = 0; i < m_pFBXPrefab->m_ObjNodeList.size(); i++)
	{
		if (m_pFBXPrefab->m_ObjNodeList[i].hasMesh)
			MeshSplitPerMaterial(i);
	}
}

bool FBXLibraryInternal::ConvertOptimize(TL_FBXLibrary::MeshInfo& meshInfo, int nodeIndex, FbxMesh* fbxMesh, tstring matName)
{
	// 서브 매시 또는 다른 매시가 있을 수도 있으니, 이미 들고 있는 매시인지 아닌지 이름을 확인하고 meshList에 넣어주자. 
	const std::string _meshName = fbxMesh->GetName();
	const tstring _meshNodeNameW = StringHelper::StringToWString(_meshName);

	auto& captureList = m_pFBXPrefab->m_MeshList;

	// meshList에 있는 data인지 검사. 
	auto FindMesh = [&captureList](const tstring& findName)
	{
		int _findIndex = 0;

		for (auto& _meshInfo : captureList)
		{
			if (_meshInfo.meshNodeName == findName)
			{
				return _findIndex;
			}
			_findIndex++;
		}

		_findIndex = -1;

		return _findIndex;
	};

	const auto _index = FindMesh(_meshNodeNameW);

	if (m_pFBXPrefab->m_MeshList.size() == 0 || _index == -1)
	{
		// vertex 최적하 하는 부분. 
		// mesh 의 종류에 사용될 union 공간을 사용할 vertex 초기화
		if (m_pFBXPrefab->m_BoneList.size() == 0)
		{
			meshInfo.isStatic = true;
			meshInfo.isAnimation = false;

			auto _isNegative = m_pFBXPrefab->m_ObjNodeList[nodeIndex].isNegative;
			//auto _nodeInfo = m_pFBXPrefab->m_ObjNodeList[nodeIndex];

			Static(meshInfo, fbxMesh, _isNegative, matName);
			//Static(meshInfo, fbxMesh, _nodeInfo, matName);
		}
		else
		{
			meshInfo.isStatic = false;
			meshInfo.isAnimation = true;

			const bool _isNegative = m_pFBXPrefab->m_ObjNodeList[nodeIndex].isNegative;

			Skinned(nodeIndex, meshInfo, fbxMesh, _isNegative, matName);

			//TL_Math::Quaternion _q = TL_Math::Quaternion::CreateFromAxisAngle(TL_Math::Vector3::Forward, -(DirectX::XM_PI / 4.0f));
			//
			//m_pFBXPrefab->m_BoneList[0].m_WorldTM *= TL_Math::Matrix::CreateFromQuaternion(_q);
		}
	}

	bool _existMesh = false;

	if (_index == -1) // 새로운 매쉬
	{
		_existMesh = true;
		return _existMesh;
	}

	return _existMesh;
}

bool FBXLibraryInternal::NonMaterialConvertOptimize(TL_FBXLibrary::MeshInfo& meshInfo, int nodeIndex, FbxMesh* fbxMesh)
{
	const std::string _meshName = fbxMesh->GetName();
	const tstring _meshNodeNameW = StringHelper::StringToWString(_meshName);

	auto& captureList = m_pFBXPrefab->m_MeshList;

	// meshList에 있는 data인지 검사. 
	auto FindMesh = [&captureList](const tstring& findName)
	{
		int _findIndex = 0;

		for (auto& _meshInfo : captureList)
		{
			if (_meshInfo.meshNodeName == findName)
			{
				return _findIndex;
			}
			_findIndex++;
		}

		_findIndex = -1;

		return _findIndex;
	};

	const auto _index = FindMesh(_meshNodeNameW);

	if (m_pFBXPrefab->m_MeshList.size() == 0 || _index == -1)
	{
		// vertex 최적하 하는 부분. 
		// mesh 의 종류에 사용될 union 공간을 사용할 vertex 초기화
		if (m_pFBXPrefab->m_BoneList.size() == 0)
		{
			meshInfo.isStatic = true;
			meshInfo.isAnimation = false;

			const bool _isNegative = m_pFBXPrefab->m_ObjNodeList[nodeIndex].isNegative;

			NonMaterialStatic(meshInfo, fbxMesh, _isNegative);
		}
		else
		{
			meshInfo.isStatic = false;
			meshInfo.isAnimation = true;

			const bool _isNegative = m_pFBXPrefab->m_ObjNodeList[nodeIndex].isNegative;

			NonMaterialSkinned(nodeIndex, meshInfo, fbxMesh, _isNegative);
		}
	}

	bool _existMesh = false;

	if (_index == -1) // 새로운 매쉬
	{
		_existMesh = true;
		return _existMesh;
	}

	return _existMesh;
}

void FBXLibraryInternal::Static(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative, tstring matName)
{
	unsigned int triCount = fbxMesh->GetPolygonCount(); // mesh 의 삼각형 개수를 가져온다.

	FbxVector4* _vertices = fbxMesh->GetControlPoints(); // control point 는 vertex 의 물리적 정보

	std::vector<TL_FBXLibrary::MyFace> optimizeFace; // index를 담을 임시버퍼 

	std::map<std::tuple<unsigned, float, float, float, float, float>, unsigned> _haveIndices; // 이미 갖고 있는 vertex인지 검사하기 위한 맵

	for (unsigned int i = 0; i < triCount; ++i) // 삼각형의 개수
	{
		TL_FBXLibrary::MyFace _pFace;
		optimizeFace.emplace_back(_pFace);

		for (unsigned int j = 0; j < 3; ++j) // 삼각형은 세 개의 정점으로 구성
		{
			const int _controlPointIndex = fbxMesh->GetPolygonVertex(i, j); // 제어점 인덱스를 가져옴.

			FbxVector4 _fbxNormal;
			fbxMesh->GetPolygonVertexNormal(i, j, _fbxNormal); // vertex의 normal 가져옴. 

			TL_Math::Vector2 _uv = GetUV(fbxMesh, i, j);

			const auto _index = std::make_tuple(_controlPointIndex, _fbxNormal.mData[0], _fbxNormal.mData[1], _fbxNormal.mData[2], _uv.x, _uv.y);

			const auto _iter = _haveIndices.find(_index);

			if (_iter == _haveIndices.end()) // map에 정보가 없다면 생성
			{
				TL_FBXLibrary::StaticVertex _pVertex;

				_pVertex.pos = TL_Math::Vector3{
					static_cast<float>(_vertices[_controlPointIndex].mData[0])
					, static_cast<float>(_vertices[_controlPointIndex].mData[2])
					, static_cast<float>(_vertices[_controlPointIndex].mData[1])
				};

				if (negative)
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(-_fbxNormal.mData[0])
						, static_cast<float>(-_fbxNormal.mData[2])
						, static_cast<float>(-_fbxNormal.mData[1])
					};
				}
				else
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(_fbxNormal.mData[0])
						, static_cast<float>(_fbxNormal.mData[2])
						, static_cast<float>(_fbxNormal.mData[1])
					};
				}

				_pVertex.uv = TL_Math::Vector2{
					_uv.x
					, _uv.y
				};

				meshInfo.StaticVertex.push_back(_pVertex);
				const uint32 _newIndex = static_cast<uint32>(meshInfo.StaticVertex.size()) - 1;
				optimizeFace[i].index[j] = _newIndex; // new Vertex의 index를 넣어줌. 

				_haveIndices.insert({ _index, _newIndex });
			}
			else // 이미 정보가 있다면 index값만 넣자.
			{
				optimizeFace[i].index[j] = _iter->second;
			}
		}
	}

	int _materialIndex = FindMaterialIndex(matName);
	assert(_materialIndex != -1);

	// subMesh가 있을 수 있으므로 material 별로 indexBuffer를 관리
	meshInfo.indexBuffer.push_back(std::make_pair(optimizeFace, _materialIndex));
	optimizeFace.clear();
}

void FBXLibraryInternal::Static(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, TL_FBXLibrary::FBXNodeInfo nodeInfo, tstring matName)
{
	unsigned int triCount = fbxMesh->GetPolygonCount(); // mesh 의 삼각형 개수를 가져온다.

	FbxVector4* _vertices = fbxMesh->GetControlPoints(); // control point 는 vertex 의 물리적 정보

	std::vector<TL_FBXLibrary::MyFace> optimizeFace; // index를 담을 임시버퍼 

	std::map<std::tuple<unsigned, float, float, float, float, float>, unsigned> _haveIndices; // 이미 갖고 있는 vertex인지 검사하기 위한 맵

	for (unsigned int i = 0; i < triCount; ++i) // 삼각형의 개수
	{
		TL_FBXLibrary::MyFace _pFace;
		optimizeFace.emplace_back(_pFace);

		for (unsigned int j = 0; j < 3; ++j) // 삼각형은 세 개의 정점으로 구성
		{
			const int _controlPointIndex = fbxMesh->GetPolygonVertex(i, j); // 제어점 인덱스를 가져옴.

			FbxVector4 _fbxNormal;
			fbxMesh->GetPolygonVertexNormal(i, j, _fbxNormal); // vertex의 normal 가져옴. 

			TL_Math::Vector2 _uv = GetUV(fbxMesh, i, j);

			const auto _index = std::make_tuple(_controlPointIndex, _fbxNormal.mData[0], _fbxNormal.mData[1], _fbxNormal.mData[2], _uv.x, _uv.y);

			const auto _iter = _haveIndices.find(_index);

			if (_iter == _haveIndices.end()) // map에 정보가 없다면 생성
			{
				// xyz -> xzy
				auto _vertexPos = ConvertVector4(_vertices[_controlPointIndex]);

				// 90도 회전한 좌표를 방영하기 위해 
				auto _newPos = Vector4Transfrom(_vertexPos, nodeInfo.nodeTM);

				TL_FBXLibrary::StaticVertex _pVertex;

				_pVertex.pos = TL_Math::Vector3(_newPos.x, _newPos.y, _newPos.z);
				//_pVertex.pos = TL_Math::Vector3{
				//	static_cast<float>(_vertices[_controlPointIndex].mData[0])
				//	, static_cast<float>(_vertices[_controlPointIndex].mData[2])
				//	, static_cast<float>(_vertices[_controlPointIndex].mData[1])
				//};

				if (nodeInfo.isNegative)
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(-_fbxNormal.mData[0])
						, static_cast<float>(-_fbxNormal.mData[2])
						, static_cast<float>(-_fbxNormal.mData[1])
					};
				}
				else
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(_fbxNormal.mData[0])
						, static_cast<float>(_fbxNormal.mData[2])
						, static_cast<float>(_fbxNormal.mData[1])
					};
				}

				_pVertex.uv = TL_Math::Vector2{
					_uv.x
					, _uv.y
				};

				meshInfo.StaticVertex.push_back(_pVertex);
				const uint32 _newIndex = static_cast<uint32>(meshInfo.StaticVertex.size()) - 1;
				optimizeFace[i].index[j] = _newIndex; // new Vertex의 index를 넣어줌. 

				_haveIndices.insert({ _index, _newIndex });
			}
			else // 이미 정보가 있다면 index값만 넣자.
			{
				optimizeFace[i].index[j] = _iter->second;
			}
		}
	}

	int _materialIndex = FindMaterialIndex(matName);
	assert(_materialIndex != -1);

	// subMesh가 있을 수 있으므로 material 별로 indexBuffer를 관리
	meshInfo.indexBuffer.push_back(std::make_pair(optimizeFace, _materialIndex));
	optimizeFace.clear();
}

void FBXLibraryInternal::Skinned(int nodeIndex, TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative, tstring matName)
{
	unsigned int triCount = fbxMesh->GetPolygonCount(); // mesh 의 삼각형 개수를 가져온다.

	FbxVector4* _vertices = fbxMesh->GetControlPoints(); // control point 는 vertex 의 물리적 정보

	std::vector<TL_FBXLibrary::MyFace> optimizeFace; // index를 담을 임시버퍼 

	std::map<std::tuple<unsigned, float, float, float, float, float>, unsigned> _haveIndices;

	for (unsigned int i = 0; i < triCount; ++i) // 삼각형의 개수
	{
		TL_FBXLibrary::MyFace _pFace;
		optimizeFace.emplace_back(_pFace);

		for (unsigned int j = 0; j < 3; ++j) // 삼각형은 세 개의 정점으로 구성
		{
			const int _controlPointIndex = fbxMesh->GetPolygonVertex(i, j); // 제어점 인덱스를 가져온다.

			FbxVector4 _fbxNormal;
			fbxMesh->GetPolygonVertexNormal(i, j, _fbxNormal);

			TL_Math::Vector2 _uv = GetUV(fbxMesh, i, j);

			TL_FBXLibrary::SkeletalVertex _pVertex;

			const auto _index = std::make_tuple(_controlPointIndex, _fbxNormal.mData[0], _fbxNormal.mData[1], _fbxNormal.mData[2], _uv.x, _uv.y);

			const auto _iter = _haveIndices.find(_index);

			if (_iter == _haveIndices.end())
			{
				_pVertex.pos = TL_Math::Vector3{
					static_cast<float>(_vertices[_controlPointIndex].mData[0])
					, static_cast<float>(_vertices[_controlPointIndex].mData[2])
					, static_cast<float>(_vertices[_controlPointIndex].mData[1])
				};

				if (negative)
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(-_fbxNormal.mData[0])
						, static_cast<float>(-_fbxNormal.mData[2])
						, static_cast<float>(-_fbxNormal.mData[1])
					};
				}
				else
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(_fbxNormal.mData[0])
						, static_cast<float>(_fbxNormal.mData[2])
						, static_cast<float>(_fbxNormal.mData[1])
					};
				}

				_pVertex.uv = TL_Math::Vector2{
					_uv.x
					,_uv.y
				};

				const TL_FBXLibrary::BoneWeight _boneWeight = m_BoneWeights[nodeIndex - 1].at(_controlPointIndex);
				_pVertex.index[0] = _boneWeight.index[0];
				_pVertex.index[1] = _boneWeight.index[1];
				_pVertex.index[2] = _boneWeight.index[2];
				_pVertex.index[3] = _boneWeight.index[3];

				_pVertex.weight1 = _boneWeight.weight1;

				meshInfo.SkeletalVertex.push_back(_pVertex);
				const uint32 _newIndex = static_cast<uint32>(meshInfo.SkeletalVertex.size()) - 1;
				optimizeFace[i].index[j] = _newIndex;
			}
			else
			{
				optimizeFace[i].index[j] = _iter->second;
			}
		}
	}

	int _materialIndex = FindMaterialIndex(matName);
	assert(_materialIndex != -1); // 매칭된 material 정보가 없다면 오류 발생.

	// subMesh가 있을 수 있으므로 material 별로 indexBuffer를 관리
	meshInfo.indexBuffer.push_back(std::make_pair(optimizeFace, _materialIndex));
	optimizeFace.clear();
}

void FBXLibraryInternal::NonMaterialStatic(TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative)
{
	unsigned int triCount = fbxMesh->GetPolygonCount(); // mesh 의 삼각형 개수를 가져온다.

	FbxVector4* _vertices = fbxMesh->GetControlPoints(); // control point 는 vertex 의 물리적 정보

	std::vector<TL_FBXLibrary::MyFace> optimizeFace; // index를 담을 임시버퍼

	std::map<std::tuple<unsigned, float, float, float, float, float>, unsigned> _haveIndices; // 이미 갖고 있는 vertex인지 검사하기 위한 맵

	for (unsigned int i = 0; i < triCount; ++i) // 삼각형의 개수
	{
		TL_FBXLibrary::MyFace _pFace;

		optimizeFace.emplace_back(_pFace);

		for (unsigned int j = 0; j < 3; ++j) // 삼각형은 세 개의 정점으로 구성
		{
			const int _controlPointIndex = fbxMesh->GetPolygonVertex(i, j); // 제어점 인덱스를 가져옴.

			FbxVector4 _fbxNormal;
			fbxMesh->GetPolygonVertexNormal(i, j, _fbxNormal); // vertex의 normal 가져옴. 

			TL_Math::Vector2 _uv = GetUV(fbxMesh, i, j);

			const auto _index = std::make_tuple(_controlPointIndex, _fbxNormal.mData[0], _fbxNormal.mData[1], _fbxNormal.mData[2], _uv.x, _uv.y);

			const auto _iter = _haveIndices.find(_index);

			if (_iter == _haveIndices.end()) // map에 정보가 없다면 생성
			{
				TL_FBXLibrary::StaticVertex _pVertex;
				_pVertex.pos = TL_Math::Vector3{
					static_cast<float>(_vertices[_controlPointIndex].mData[0])
					, static_cast<float>(_vertices[_controlPointIndex].mData[2])
					, static_cast<float>(_vertices[_controlPointIndex].mData[1])
				};

				if (negative)
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(-_fbxNormal.mData[0])
						, static_cast<float>(-_fbxNormal.mData[2])
						, static_cast<float>(-_fbxNormal.mData[1])
					};
				}
				else
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(_fbxNormal.mData[0])
						, static_cast<float>(_fbxNormal.mData[2])
						, static_cast<float>(_fbxNormal.mData[1])
					};
				}

				_pVertex.uv = TL_Math::Vector2{
					_uv.x
					, _uv.y
				};

				meshInfo.StaticVertex.push_back(_pVertex);
				const uint32 _newIndex = static_cast<uint32>(meshInfo.StaticVertex.size()) - 1;
				optimizeFace[i].index[j] = _newIndex; // new Vertex의 index를 넣어줌. 

				_haveIndices.insert({ _index, _newIndex });
			}
			else // 이미 정보가 있다면 index값만 넣자.
			{
				optimizeFace[i].index[j] = _iter->second;
			}
		}
	}

	int _materialIndex = -1;
	// subMesh가 있을 수 있으므로 material 별로 indexBuffer를 관리
	meshInfo.indexBuffer.push_back(std::make_pair(optimizeFace, _materialIndex));
	optimizeFace.clear();
}

void FBXLibraryInternal::NonMaterialSkinned(int nodeIndex, TL_FBXLibrary::MeshInfo& meshInfo, FbxMesh* fbxMesh, bool negative)
{
	unsigned int triCount = fbxMesh->GetPolygonCount(); // mesh 의 삼각형 개수를 가져온다.

	FbxVector4* _vertices = fbxMesh->GetControlPoints(); // control point 는 vertex 의 물리적 정보

	std::vector<TL_FBXLibrary::MyFace> optimizeFace; // index를 담을 임시버퍼 

	std::map<std::tuple<unsigned, float, float, float, float, float>, unsigned> _haveIndices;

	for (unsigned int i = 0; i < triCount; ++i) // 삼각형의 개수
	{
		TL_FBXLibrary::MyFace _pFace;
		optimizeFace.emplace_back(_pFace);

		for (unsigned int j = 0; j < 3; ++j) // 삼각형은 세 개의 정점으로 구성
		{
			const int _controlPointIndex = fbxMesh->GetPolygonVertex(i, j); // 제어점 인덱스를 가져온다.

			FbxVector4 _fbxNormal;
			fbxMesh->GetPolygonVertexNormal(i, j, _fbxNormal);

			TL_Math::Vector2 _uv = GetUV(fbxMesh, i, j);

			TL_FBXLibrary::SkeletalVertex _pVertex;

			const auto _index = std::make_tuple(_controlPointIndex, _fbxNormal.mData[0], _fbxNormal.mData[1], _fbxNormal.mData[2], _uv.x, _uv.y);

			const auto _iter = _haveIndices.find(_index);

			if (_iter == _haveIndices.end())
			{
				_pVertex.pos = TL_Math::Vector3{
					static_cast<float>(_vertices[_controlPointIndex].mData[0])
					, static_cast<float>(_vertices[_controlPointIndex].mData[2])
					, static_cast<float>(_vertices[_controlPointIndex].mData[1])
				};

				if (negative)
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(-_fbxNormal.mData[0])
						, static_cast<float>(-_fbxNormal.mData[2])
						, static_cast<float>(-_fbxNormal.mData[1])
					};
				}
				else
				{
					_pVertex.normal = TL_Math::Vector3{
						static_cast<float>(_fbxNormal.mData[0])
						, static_cast<float>(_fbxNormal.mData[2])
						, static_cast<float>(_fbxNormal.mData[1])
					};
				}

				_pVertex.uv = TL_Math::Vector2{
					_uv.x
					,_uv.y
				};

				const TL_FBXLibrary::BoneWeight _boneWeight = m_BoneWeights[nodeIndex].at(_controlPointIndex);
				_pVertex.index[0] = _boneWeight.index[0];
				_pVertex.index[1] = _boneWeight.index[1];
				_pVertex.index[2] = _boneWeight.index[2];
				_pVertex.index[3] = _boneWeight.index[3];

				_pVertex.weight1 = _boneWeight.weight1;

				meshInfo.SkeletalVertex.push_back(_pVertex);
				const uint32 _newIndex = static_cast<uint32>(meshInfo.SkeletalVertex.size()) - 1;
				optimizeFace[i].index[j] = _newIndex;
			}
			else
			{
				optimizeFace[i].index[j] = _iter->second;
			}
		}
	}

	int _materialIndex = -1;

	// subMesh가 있을 수 있으므로 material 별로 indexBuffer를 관리
	meshInfo.indexBuffer.push_back(std::make_pair(optimizeFace, _materialIndex));
	optimizeFace.clear();
}

void FBXLibraryInternal::MeshSplitPerMaterial(int nodeIndex)
{
	/// SubMesh 정리
	///	1. 여러 material를 사용하는 Mesh를 material 기준으로 1:1 매칭하여 쪼갬 (load 함수 안에서 이미 함.)
	///	2. 쪼개진 Mesh를 돌면서 vertex 최적화
	///	3. index Buffer에 넣어주기.
	///	4. 쪼개진 VertexBuffer 합쳐주기.

	FbxMesh* _pMesh = m_FBXNodeHierarchy.at(nodeIndex)->m_pFBXMyNode->GetMesh();

	// _meshCount는 mesh와 material이 1:1 매칭이 되었으면 1개, subMesh가 있다면 2개 이상의 값이 나옴.
	const int _meshCount = m_FBXNodeHierarchy.at(nodeIndex)->m_pFBXMyNode->GetNodeAttributeCount();

	TL_FBXLibrary::MeshInfo _meshInfo;

	// 처음 mesh의 이름을 넣어주자. 
	_meshInfo.meshNodeName = StringHelper::StringToWString(_pMesh->GetName());

	bool _isNewMesh = false; // 이미 meshList에 있는 mesh라면? 

	for (int i = 0; i < _meshCount; i++)
	{
		// mesh가 사용하고 있는 material 정보에 접근.
		FbxSurfaceMaterial* _fbxMaterial = m_FBXNodeHierarchy.at(nodeIndex)->m_pFBXMyNode->GetSrcObject<FbxSurfaceMaterial>(i);

		LoadMaterial(_meshInfo, _fbxMaterial, m_pFBXPrefab->m_MaterialList); // mesh에 따라 1:1 매칭

		if (_meshInfo.isMaterial == true)
		{
			tstring _matName = StringHelper::StringToWString(_fbxMaterial->GetName());

			// 쪼개진 mesh에 접근하기 위한 코드 
			FbxMesh* _mesh = (FbxMesh*)m_FBXNodeHierarchy.at(nodeIndex)->m_pFBXMyNode->GetNodeAttributeByIndex(i);

			_isNewMesh = ConvertOptimize(_meshInfo, nodeIndex, _mesh, _matName);
		}
		else
		{
			_isNewMesh = NonMaterialConvertOptimize(_meshInfo, nodeIndex, _pMesh);
		}
	}

	if (_isNewMesh == true)
	{
		PushTangentAndBiTangent(_meshInfo);

		m_pFBXPrefab->m_MeshList.push_back(_meshInfo);
	}
}

void FBXLibraryInternal::LoadMaterial(TL_FBXLibrary::MeshInfo& meshInfo, FbxSurfaceMaterial* fbxMaterialData, std::vector<TL_FBXLibrary::FBXMaterialInfo>& matInfo)
{
	if (fbxMaterialData == nullptr)
	{
		return;
	}

	meshInfo.isMaterial = true;

	TL_FBXLibrary::FBXMaterialInfo _materialInfo;

	tstring _matName = StringHelper::StringToWString(fbxMaterialData->GetName());
	_materialInfo.materialName = _matName;

	bool _foundMat = false;
	uint32 _matIndex = 0;
	for (auto& _matInfo : matInfo)
	{
		if (_matInfo.materialName == _matName)
		{
			_foundMat = true;

			meshInfo.materialIndexs.push_back(_matIndex);
		}

		_matIndex++;
	}

	// 이미 materialList에 정보가 있다면 함수를 나가자. 
	if (_foundMat == true) return;

	// phong 모델
	if (fbxMaterialData->GetClassId().Is(FbxSurfacePhong::ClassId))
	{
		FbxSurfacePhong* _phongMaterial = (FbxSurfacePhong*)(fbxMaterialData);


		_materialInfo.ambient[0] = static_cast<float>(_phongMaterial->Ambient.Get()[0]) * 10.0f;
		_materialInfo.ambient[1] = static_cast<float>(_phongMaterial->Ambient.Get()[1]) * 10.0f;
		_materialInfo.ambient[2] = static_cast<float>(_phongMaterial->Ambient.Get()[2]) * 10.0f;
		_materialInfo.ambient[3] = 1.0f;

		_materialInfo.diffuse[0] = static_cast<float>(_phongMaterial->Diffuse.Get()[0]);
		_materialInfo.diffuse[1] = static_cast<float>(_phongMaterial->Diffuse.Get()[1]);
		_materialInfo.diffuse[2] = static_cast<float>(_phongMaterial->Diffuse.Get()[2]);
		_materialInfo.diffuse[3] = 1.0f;

		_materialInfo.specular[0] = static_cast<float>(_phongMaterial->Specular.Get()[0]);
		_materialInfo.specular[1] = static_cast<float>(_phongMaterial->Specular.Get()[1]);
		_materialInfo.specular[2] = static_cast<float>(_phongMaterial->Specular.Get()[2]);
		_materialInfo.specular[3] = 1.0f;

		_materialInfo.emissive[0] = static_cast<float>(_phongMaterial->Emissive.Get()[0]);
		_materialInfo.emissive[1] = static_cast<float>(_phongMaterial->Emissive.Get()[1]);
		_materialInfo.emissive[2] = static_cast<float>(_phongMaterial->Emissive.Get()[2]);
		_materialInfo.emissive[3] = 1.0f;

		_materialInfo.shininess = static_cast<float>(_phongMaterial->Shininess.Get());

		_materialInfo.roughness = 1.f - (sqrt(std::max(_materialInfo.shininess, 0.f)) / 10.f);

		_materialInfo.transparency = 1.0f - static_cast<float>(_phongMaterial->TransparencyFactor.Get());

		_materialInfo.reflectivity = static_cast<float>(_phongMaterial->ReflectionFactor.Get());
	}
	else if (fbxMaterialData->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		FbxSurfaceLambert* _LambertMaterial = (FbxSurfaceLambert*)(fbxMaterialData);

		_materialInfo.ambient[0] = static_cast<float>(_LambertMaterial->Ambient.Get()[0]);
		_materialInfo.ambient[1] = static_cast<float>(_LambertMaterial->Ambient.Get()[1]);
		_materialInfo.ambient[2] = static_cast<float>(_LambertMaterial->Ambient.Get()[2]);
		_materialInfo.ambient[3] = 1.0f;

		_materialInfo.diffuse[0] = static_cast<float>(_LambertMaterial->Diffuse.Get()[0]);
		_materialInfo.diffuse[1] = static_cast<float>(_LambertMaterial->Diffuse.Get()[1]);
		_materialInfo.diffuse[2] = static_cast<float>(_LambertMaterial->Diffuse.Get()[2]);
		_materialInfo.diffuse[3] = 1.0f;

		_materialInfo.emissive[0] = static_cast<float>(_LambertMaterial->Emissive.Get()[0]);
		_materialInfo.emissive[1] = static_cast<float>(_LambertMaterial->Emissive.Get()[1]);
		_materialInfo.emissive[2] = static_cast<float>(_LambertMaterial->Emissive.Get()[2]);
		_materialInfo.emissive[3] = 1.0f;

		_materialInfo.transparency = 1.0f - static_cast<float>(_LambertMaterial->TransparencyFactor.Get());
	}

	FbxProperty _propsAmbient = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sAmbient);
	_materialInfo.ambientFile = LoadFBXTextureProperty(_propsAmbient);

	FbxProperty _propDiffuse = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sDiffuse);
	_materialInfo.baseColorFile = LoadFBXTextureProperty(_propDiffuse);

	FbxProperty _propRoughness = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sShininess);
	_materialInfo.roughnessMapFile = LoadFBXTextureProperty(_propRoughness);

	FbxProperty _propSpecular = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sSpecular);
	_materialInfo.specularFile = LoadFBXTextureProperty(_propSpecular);

	FbxProperty _propEmissive = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sEmissive);
	_materialInfo.emissiveFile = LoadFBXTextureProperty(_propEmissive);

	FbxProperty _propNormalMap = fbxMaterialData->FindProperty(FbxSurfaceMaterial::sNormalMap);
	_materialInfo.normalMapFile = LoadFBXTextureProperty(_propNormalMap);

	_materialInfo.materialIndex = matInfo.size();

	meshInfo.materialIndexs.push_back(matInfo.size()); // size를 넣으면 푸쉬백 하기전의 인덱스 나옴

	matInfo.push_back(_materialInfo);
}

int FBXLibraryInternal::GetObjMeshListSize()
{
	if (m_pFBXPrefab->m_ObjNodeList.size() == 0) return -1;

	return static_cast<int>(m_pFBXPrefab->m_ObjNodeList.size());
}

void FBXLibraryInternal::TextSerialize()
{
	// create and open a character archive for output
	std::ofstream ofs("TEXT");

	// create class instance
	std::string _prefapName = StringHelper::WStringToString(m_pFBXPrefab->name);
	PrefabData _prefab(_prefapName);

	// save data to archive
	boost::archive::binary_oarchive oa(ofs);

	// 머터리얼 데이터 뽑기
	for (auto _material : m_pFBXPrefab->m_MaterialList)
	{
		std::string _materialName = StringHelper::WStringToString(_material.materialName);

		MaterialData _materialData(_material.materialIndex
			, _materialName
			, { _material.ambient[0], _material.ambient[1], _material.ambient[2], _material.ambient[3] }
			, { _material.diffuse[0], _material.diffuse[1], _material.diffuse[2], _material.diffuse[3] }
			, { _material.specular[0], _material.specular[1], _material.specular[2], _material.specular[3] }
			, { _material.emissive[0], _material.emissive[1], _material.emissive[2], _material.emissive[3] }
			, _material.roughness
			, _material.shininess
			, _material.transparency
			, _material.reflectivity
			, StringHelper::WStringToString(_material.baseColorFile)
			, StringHelper::WStringToString(_material.normalMapFile)
			, StringHelper::WStringToString(_material.roughnessMapFile)
			, StringHelper::WStringToString(_material.emissiveFile)
			, StringHelper::WStringToString(_material.ambientFile)
			, StringHelper::WStringToString(_material.specularFile)
		);

		_prefab.m_MaterialInfoList.push_back(_materialData);
	}

	// mesh data 뽑기 
	for (auto _mesh : m_pFBXPrefab->m_MeshList)
	{
		MeshData _meshData(StringHelper::WStringToString(_mesh.meshNodeName)
			, _mesh.isMaterial
			, _mesh.isStatic
			, _mesh.isAnimation);

		for (auto _index : _mesh.materialIndexs)
		{
			_meshData.m_Materials.push_back(_index);
		}

		if (_mesh.StaticVertex.size() != 0)
		{
			for (auto _vertex : _mesh.StaticVertex)
			{
				const StaticVertex _staticVertex({ _vertex.pos.x, _vertex.pos.y, _vertex.pos.z }
					, { _vertex.uv.x, _vertex.uv.y }
					, { _vertex.normal.x, _vertex.normal.y, _vertex.normal.z }
					, { _vertex.tangent.x, _vertex.tangent.y, _vertex.tangent.z }
				, { _vertex.bitangent.x,  _vertex.bitangent.y,  _vertex.bitangent.z });

				_meshData.m_StaticVertex.push_back(_staticVertex);
			}
		}
		else
		{
			for (auto _vertex : _mesh.SkeletalVertex)
			{
				const SkeletalVertex _skeletalVertex({ _vertex.pos.x, _vertex.pos.y, _vertex.pos.z }
					, { _vertex.uv.x, _vertex.uv.y }
					, { _vertex.normal.x, _vertex.normal.y, _vertex.normal.z }
					, { _vertex.tangent.x, _vertex.tangent.y, _vertex.tangent.z }
					, { _vertex.bitangent.x,  _vertex.bitangent.y,  _vertex.bitangent.z }
					, { _vertex.index[0],  _vertex.index[1],  _vertex.index[2], _vertex.index[3] }
				, { _vertex.weight1.x,  _vertex.weight1.y,  _vertex.weight1.z, _vertex.weight1.w });

				_meshData.m_SkeletalVertex.push_back(_skeletalVertex);
			}
		}

		for (auto _buffer : _mesh.indexBuffer)
		{
			std::vector<MyFace> _faceBuffer;

			for (auto _face : _buffer.first)
			{
				const MyFace face(_face.index[0], _face.index[1], _face.index[2]);

				_faceBuffer.push_back(face);
			}

			_meshData.m_IndexBuffer.push_back(std::make_pair(_faceBuffer, _buffer.second));
		}

		_prefab.m_MeshDataList.push_back(_meshData);
	}

	for (auto _Node : m_pFBXPrefab->m_ObjNodeList)
	{
		std::string _nodeName = StringHelper::WStringToString(_Node.nodeName);
		std::string _parentName = StringHelper::WStringToString(_Node.parentNodeName);
		std::string _meshName = StringHelper::WStringToString(_Node.meshNodeName);

		const auto _nodeTM = ConvertToFloat4X4(_Node.nodeTM);

		const NodeData _NodeData(_nodeName, _parentName, _meshName, _Node.hasMesh, _Node.isNegative, _nodeTM);;

		_prefab.m_NodeList.push_back(_NodeData);
	}

	for (auto _skeleton : m_pFBXPrefab->m_BoneList)
	{
		std::string _boneName = StringHelper::WStringToString(_skeleton.boneName);
		std::string _parentBoneName = StringHelper::WStringToString(_skeleton.parentboneName);

		const auto _worldTM = ConvertToFloat4X4(_skeleton.m_WorldTM);
		const auto _offsetTM = ConvertToFloat4X4(_skeleton.m_bone_From3DSceneSpaceToBoneSpaceTM);

		const MySkeleton _boneData(_boneName, _parentBoneName, _skeleton.parentBoneIndex, _worldTM, _offsetTM);

		_prefab.m_BoneList.push_back(_boneData);
	}

	for (auto _ani : m_pFBXPrefab->m_AnimList)
	{
		std::string _aniName = StringHelper::WStringToString(_ani.name);
		std::string _aniTypeName = StringHelper::WStringToString(_ani.typeName);

		Animation _aniData(_aniName, _aniTypeName, _ani.frameRate, _ani.tickPerFrame, _ani.totalKeyFrame, _ani.startKeyFrame, _ani.endKeyFrame);

		for (auto _frameInfo : _ani.keyFrameInfo)
		{
			std::string _nodeName = StringHelper::WStringToString(_frameInfo.nodeName);
			std::string _boneName = StringHelper::WStringToString(_frameInfo.boneName);

			KeyFrameInfo _frame(_nodeName, _boneName);

			for (auto _Info : _frameInfo.animInfo)
			{
				int time = _Info.time;
				Float3 pos{ _Info.pos.x,_Info.pos.y, _Info.pos.z };
				Float4 rot{ _Info.rot.x, _Info.rot.y, _Info.rot.z, _Info.rot.w };
				Float3 scl{ _Info.scl.x, _Info.scl.y, _Info.scl.z };

				const AnimationData _animInfo(time, pos, rot, scl);

				_frame.m_AnimData.push_back(_animInfo);
			}

			_aniData.m_KeyFrameInfo.push_back(_frame);
		}

		_prefab.m_AnimList.push_back(_aniData);
	}

	oa << _prefab;
}

void FBXLibraryInternal::BinarySerialize()
{
	// create class instance
	std::string _prefapName = StringHelper::WStringToString(m_pFBXPrefab->name);
	PrefabData _prefab(_prefapName);

	// 파일 포맷 변경
	std::string modExt = "TL";

	int ext = _prefapName.rfind("fbx");
	int name = _prefapName.rfind("/") + 1;

	std::string dstPath = _prefapName.substr(0, name);
	dstPath += _prefapName.substr(name, ext - name);
	dstPath += modExt;

	// create and open a character archive for output
	std::ofstream ofs(dstPath, std::ios::binary);

	// save data to archive
	boost::archive::binary_oarchive oa(ofs);

	for (auto _material : m_pFBXPrefab->m_MaterialList)
	{
		std::string _materialName = StringHelper::WStringToString(_material.materialName);

		MaterialData _materialData(_material.materialIndex
			, _materialName
			, { _material.ambient[0], _material.ambient[1], _material.ambient[2], _material.ambient[3] }
			, { _material.diffuse[0], _material.diffuse[1], _material.diffuse[2], _material.diffuse[3] }
			, { _material.specular[0], _material.specular[1], _material.specular[2], _material.specular[3] }
			, { _material.emissive[0], _material.emissive[1], _material.emissive[2], _material.emissive[3] }
			, _material.roughness
			, _material.shininess
			, _material.transparency
			, _material.reflectivity
			, StringHelper::WStringToString(_material.baseColorFile)
			, StringHelper::WStringToString(_material.normalMapFile)
			, StringHelper::WStringToString(_material.roughnessMapFile)
			, StringHelper::WStringToString(_material.emissiveFile)
			, StringHelper::WStringToString(_material.ambientFile)
			, StringHelper::WStringToString(_material.specularFile)
		);

		_prefab.m_MaterialInfoList.push_back(_materialData);
	}

	// mesh data 뽑기 
	for (auto _mesh : m_pFBXPrefab->m_MeshList)
	{
		MeshData _meshData(StringHelper::WStringToString(_mesh.meshNodeName)
			, _mesh.isMaterial
			, _mesh.isStatic
			, _mesh.isAnimation);

		for (auto _index : _mesh.materialIndexs)
		{
			_meshData.m_Materials.push_back(_index);
		}

		if (_mesh.StaticVertex.size() != 0)
		{
			for (auto _vertex : _mesh.StaticVertex)
			{
				const StaticVertex _staticVertex({ _vertex.pos.x, _vertex.pos.y, _vertex.pos.z }
					, { _vertex.uv.x, _vertex.uv.y }
					, { _vertex.normal.x, _vertex.normal.y, _vertex.normal.z }
					, { _vertex.tangent.x, _vertex.tangent.y, _vertex.tangent.z }
				, { _vertex.bitangent.x,  _vertex.bitangent.y,  _vertex.bitangent.z });

				_meshData.m_StaticVertex.push_back(_staticVertex);
			}
		}
		else
		{
			for (auto _vertex : _mesh.SkeletalVertex)
			{
				const SkeletalVertex _skeletalVertex({ _vertex.pos.x, _vertex.pos.y, _vertex.pos.z }
					, { _vertex.uv.x, _vertex.uv.y }
					, { _vertex.normal.x, _vertex.normal.y, _vertex.normal.z }
					, { _vertex.tangent.x, _vertex.tangent.y, _vertex.tangent.z }
					, { _vertex.bitangent.x,  _vertex.bitangent.y,  _vertex.bitangent.z }
					, { _vertex.index[0],  _vertex.index[1],  _vertex.index[2], _vertex.index[3] }
				, { _vertex.weight1.x,  _vertex.weight1.y,  _vertex.weight1.z, _vertex.weight1.w });

				_meshData.m_SkeletalVertex.push_back(_skeletalVertex);
			}
		}

		for (auto _buffer : _mesh.indexBuffer)
		{
			std::vector<MyFace> _faceBuffer;

			for (auto _face : _buffer.first)
			{
				const MyFace face(_face.index[0], _face.index[1], _face.index[2]);

				_faceBuffer.push_back(face);
			}

			_meshData.m_IndexBuffer.push_back(std::make_pair(_faceBuffer, _buffer.second));
		}

		_prefab.m_MeshDataList.push_back(_meshData);
	}

	for (auto _Node : m_pFBXPrefab->m_ObjNodeList)
	{
		std::string _nodeName = StringHelper::WStringToString(_Node.nodeName);
		std::string _parentName = StringHelper::WStringToString(_Node.parentNodeName);
		std::string _meshName = StringHelper::WStringToString(_Node.meshNodeName);

		const auto _nodeTM = ConvertToFloat4X4(_Node.nodeTM);

		const NodeData _NodeData(_nodeName, _parentName, _meshName, _Node.hasMesh, _Node.isNegative, _nodeTM);;

		_prefab.m_NodeList.push_back(_NodeData);
	}

	for (auto _skeleton : m_pFBXPrefab->m_BoneList)
	{
		std::string _boneName = StringHelper::WStringToString(_skeleton.boneName);
		std::string _parentBoneName = StringHelper::WStringToString(_skeleton.parentboneName);

		const auto _worldTM = ConvertToFloat4X4(_skeleton.m_WorldTM);
		const auto _offsetTM = ConvertToFloat4X4(_skeleton.m_bone_From3DSceneSpaceToBoneSpaceTM);

		const MySkeleton _boneData(_boneName, _parentBoneName, _skeleton.parentBoneIndex, _worldTM, _offsetTM);

		_prefab.m_BoneList.push_back(_boneData);
	}

	for (auto _ani : m_pFBXPrefab->m_AnimList)
	{
		std::string _aniName = StringHelper::WStringToString(_ani.name);
		std::string _aniTypeName = StringHelper::WStringToString(_ani.typeName);

		Animation _aniData(_aniName, _aniTypeName, _ani.frameRate, _ani.tickPerFrame, _ani.totalKeyFrame, _ani.startKeyFrame, _ani.endKeyFrame);

		for (auto _frameInfo : _ani.keyFrameInfo)
		{
			std::string _nodeName = StringHelper::WStringToString(_frameInfo.nodeName);
			std::string _boneName = StringHelper::WStringToString(_frameInfo.boneName);

			KeyFrameInfo _frame(_nodeName, _boneName);

			for (auto _Info : _frameInfo.animInfo)
			{
				float time = _Info.time;
				Float3 pos{ _Info.pos.x,_Info.pos.y, _Info.pos.z };
				Float4 rot{ _Info.rot.x, _Info.rot.y, _Info.rot.z, _Info.rot.w };
				Float3 scl{ _Info.scl.x, _Info.scl.y, _Info.scl.z };

				const AnimationData _animInfo(time, pos, rot, scl);

				_frame.m_AnimData.push_back(_animInfo);
			}

			_aniData.m_KeyFrameInfo.push_back(_frame);
		}

		_prefab.m_AnimList.push_back(_aniData);
	}

	oa << _prefab;
}

bool FBXLibraryInternal::PushBinaryData(PrefabData preFab)
{
	m_pFBXPrefab = std::make_unique<TL_FBXLibrary::FBXPrefab>();
	//m_bineryFrefab = new TL_FBXLibrary::FBXPrefab();

	m_pFBXPrefab->name = StringHelper::StringToWString(preFab.m_PrefabName);

	if (m_pFBXPrefab->name.size() == 0) return false; // 파일이 없으면 false 반환

	for (auto _material : preFab.m_MaterialInfoList)
	{
		TL_FBXLibrary::FBXMaterialInfo _newMaterial;
		_newMaterial.materialIndex = _material.m_MaterialIndex;
		_newMaterial.materialName = StringHelper::StringToWString(_material.m_MaterialName);

		for (int i = 0; i < 4; ++i)
		{
			_newMaterial.ambient[i] = _material.m_Ambient[i];
			_newMaterial.diffuse[i] = _material.m_Diffuse[i];
			_newMaterial.specular[i] = _material.m_Specular[i];
			_newMaterial.emissive[i] = _material.m_Emissive[i];
		}

		_newMaterial.roughness = _material.m_Roughness;
		_newMaterial.shininess = _material.m_Shininess;
		_newMaterial.transparency = _material.m_Transparency;
		_newMaterial.reflectivity = _material.m_Reflectivity;

		_newMaterial.baseColorFile = StringHelper::StringToWString(_material.m_BaseColorFile);
		_newMaterial.normalMapFile = StringHelper::StringToWString(_material.m_NormalMapFile);
		_newMaterial.roughnessMapFile = StringHelper::StringToWString(_material.m_RoughnessMapFile);
		_newMaterial.emissiveFile = StringHelper::StringToWString(_material.m_EmissiveFile);
		_newMaterial.ambientFile = StringHelper::StringToWString(_material.m_AmbientFile);
		_newMaterial.specularFile = StringHelper::StringToWString(_material.m_SpecularFile);

		m_pFBXPrefab->m_MaterialList.push_back(_newMaterial);
	}

	for (auto _mesh : preFab.m_MeshDataList)
	{
		TL_FBXLibrary::MeshInfo _newMesh;

		_newMesh.meshNodeName = StringHelper::StringToWString(_mesh.m_MeshNodeName);
		_newMesh.isMaterial = _mesh.m_IsMaterial;
		_newMesh.isStatic = _mesh.m_IsStatic;
		_newMesh.isAnimation = _mesh.m_IsAnimation;

		_newMesh.materialIndexs = _mesh.m_Materials;

		if (_mesh.m_StaticVertex.size() != 0)
		{
			for (auto _vertex : _mesh.m_StaticVertex)
			{
				TL_FBXLibrary::StaticVertex _newVertex;

				_newVertex.pos.x = _vertex.m_Pos.m_x;
				_newVertex.pos.y = _vertex.m_Pos.m_y;
				_newVertex.pos.z = _vertex.m_Pos.m_z;

				_newVertex.uv.x = _vertex.m_UV.m_x;
				_newVertex.uv.y = _vertex.m_UV.m_y;

				_newVertex.normal.x = _vertex.m_Normal.m_x;
				_newVertex.normal.y = _vertex.m_Normal.m_y;
				_newVertex.normal.z = _vertex.m_Normal.m_z;

				_newVertex.tangent.x = _vertex.m_Tangent.m_x;
				_newVertex.tangent.y = _vertex.m_Tangent.m_y;
				_newVertex.tangent.z = _vertex.m_Tangent.m_z;

				_newVertex.bitangent.x = _vertex.m_BiTangent.m_x;
				_newVertex.bitangent.y = _vertex.m_BiTangent.m_y;
				_newVertex.bitangent.z = _vertex.m_BiTangent.m_z;

				_newMesh.StaticVertex.push_back(_newVertex);
			}
		}
		else
		{
			for (auto _vertex : _mesh.m_SkeletalVertex)
			{
				TL_FBXLibrary::SkeletalVertex _newVertex;

				_newVertex.pos.x = _vertex.m_Pos.m_x;
				_newVertex.pos.y = _vertex.m_Pos.m_y;
				_newVertex.pos.z = _vertex.m_Pos.m_z;

				_newVertex.uv.x = _vertex.m_UV.m_x;
				_newVertex.uv.y = _vertex.m_UV.m_y;

				_newVertex.normal.x = _vertex.m_Normal.m_x;
				_newVertex.normal.y = _vertex.m_Normal.m_y;
				_newVertex.normal.z = _vertex.m_Normal.m_z;

				_newVertex.tangent.x = _vertex.m_Tangent.m_x;
				_newVertex.tangent.y = _vertex.m_Tangent.m_y;
				_newVertex.tangent.z = _vertex.m_Tangent.m_z;

				_newVertex.bitangent.x = _vertex.m_BiTangent.m_x;
				_newVertex.bitangent.y = _vertex.m_BiTangent.m_y;
				_newVertex.bitangent.z = _vertex.m_BiTangent.m_z;

				for (int i = 0; i < 4; ++i)
				{
					_newVertex.index[i] = _vertex.m_Index[i];
				}

				_newVertex.weight1.x = _vertex.m_Weight1.m_x;
				_newVertex.weight1.y = _vertex.m_Weight1.m_y;
				_newVertex.weight1.z = _vertex.m_Weight1.m_z;
				_newVertex.weight1.w = _vertex.m_Weight1.m_w;

				_newMesh.SkeletalVertex.push_back(_newVertex);
			}
		}

		for (auto _buffer : _mesh.m_IndexBuffer)
		{
			std::vector<TL_FBXLibrary::MyFace> _faceBuffer;

			for (auto _face : _buffer.first)
			{
				TL_FBXLibrary::MyFace face;

				face.index[0] = _face.m_x;
				face.index[1] = _face.m_y;
				face.index[2] = _face.m_z;

				_faceBuffer.push_back(face);
			}
			_newMesh.indexBuffer.push_back(std::make_pair(_faceBuffer, _buffer.second));
		}

		m_pFBXPrefab->m_MeshList.push_back(_newMesh);
	}

	for (auto _node : preFab.m_NodeList)
	{
		TL_FBXLibrary::FBXNodeInfo _newNode;

		_newNode.nodeName = StringHelper::StringToWString(_node.m_NodeName);
		_newNode.parentNodeName = StringHelper::StringToWString(_node.m_ParentNodeName);
		_newNode.meshNodeName = StringHelper::StringToWString(_node.m_MeshNodeName);

		_newNode.hasMesh = _node.m_HasMesh;
		_newNode.isNegative = _node.m_IsNegative;

		TL_Math::Matrix _newNodeTM(_node.m_NodeTM.m_11, _node.m_NodeTM.m_12, _node.m_NodeTM.m_13, _node.m_NodeTM.m_14
			, _node.m_NodeTM.m_21, _node.m_NodeTM.m_22, _node.m_NodeTM.m_23, _node.m_NodeTM.m_24
			, _node.m_NodeTM.m_31, _node.m_NodeTM.m_32, _node.m_NodeTM.m_33, _node.m_NodeTM.m_34
			, _node.m_NodeTM.m_41, _node.m_NodeTM.m_42, _node.m_NodeTM.m_43, _node.m_NodeTM.m_44);
		_newNode.nodeTM = _newNodeTM;

		m_pFBXPrefab->m_ObjNodeList.push_back(_newNode);
	}

	for (auto _bone : preFab.m_BoneList)
	{
		TL_FBXLibrary::MySkeleton _newBone;

		_newBone.boneName = StringHelper::StringToWString(_bone.m_BoneName);
		_newBone.parentboneName = StringHelper::StringToWString(_bone.m_ParentBoneName);

		_newBone.parentBoneIndex = _bone.m_ParentBoneIndex;

		TL_Math::Matrix _newWorldTM(_bone.m_WorldTM.m_11, _bone.m_WorldTM.m_12, _bone.m_WorldTM.m_13, _bone.m_WorldTM.m_14
			, _bone.m_WorldTM.m_21, _bone.m_WorldTM.m_22, _bone.m_WorldTM.m_23, _bone.m_WorldTM.m_24
			, _bone.m_WorldTM.m_31, _bone.m_WorldTM.m_32, _bone.m_WorldTM.m_33, _bone.m_WorldTM.m_34
			, _bone.m_WorldTM.m_41, _bone.m_WorldTM.m_42, _bone.m_WorldTM.m_43, _bone.m_WorldTM.m_44);
		_newBone.m_WorldTM = _newWorldTM;

		TL_Math::Matrix _newOffsetTM(_bone.m_OffestTM.m_11, _bone.m_OffestTM.m_12, _bone.m_OffestTM.m_13, _bone.m_OffestTM.m_14
			, _bone.m_OffestTM.m_21, _bone.m_OffestTM.m_22, _bone.m_OffestTM.m_23, _bone.m_OffestTM.m_24
			, _bone.m_OffestTM.m_31, _bone.m_OffestTM.m_32, _bone.m_OffestTM.m_33, _bone.m_OffestTM.m_34
			, _bone.m_OffestTM.m_41, _bone.m_OffestTM.m_42, _bone.m_OffestTM.m_43, _bone.m_OffestTM.m_44);

		_newBone.m_bone_From3DSceneSpaceToBoneSpaceTM = _newOffsetTM;

		m_pFBXPrefab->m_BoneList.push_back(_newBone);
	}

	for (auto _anim : preFab.m_AnimList)
	{
		TL_FBXLibrary::Animation _newAnim;

		_newAnim.name = StringHelper::StringToWString(_anim.m_Name);
		_newAnim.typeName = StringHelper::StringToWString(_anim.m_TypeName);

		_newAnim.frameRate = _anim.m_FrameRate;
		_newAnim.tickPerFrame = _anim.m_TickPerFrame;
		_newAnim.totalKeyFrame = _anim.m_TotalKeyFrame;
		_newAnim.startKeyFrame = _anim.m_StartKeyFrame;
		_newAnim.endKeyFrame = _anim.m_EndKeyFrame;

		for (auto _keyFrame : _anim.m_KeyFrameInfo)
		{
			TL_FBXLibrary::keyFrameInfo _newKeyFrame;

			_newKeyFrame.nodeName = StringHelper::StringToWString(_keyFrame.m_NodeName);
			_newKeyFrame.boneName = StringHelper::StringToWString(_keyFrame.m_BoneName);

			for (auto _animInfo : _keyFrame.m_AnimData)
			{
				TL_FBXLibrary::AnimationInfo _newAnimInfo;

				_newAnimInfo.time = _animInfo.m_Time;

				_newAnimInfo.pos.x = _animInfo.m_Pos.m_x;
				_newAnimInfo.pos.y = _animInfo.m_Pos.m_y;
				_newAnimInfo.pos.z = _animInfo.m_Pos.m_z;

				_newAnimInfo.rot.x = _animInfo.m_Rot.m_x;
				_newAnimInfo.rot.y = _animInfo.m_Rot.m_y;
				_newAnimInfo.rot.z = _animInfo.m_Rot.m_z;
				_newAnimInfo.rot.w = _animInfo.m_Rot.m_w;

				_newAnimInfo.scl.x = _animInfo.m_Scl.m_x;
				_newAnimInfo.scl.y = _animInfo.m_Scl.m_y;
				_newAnimInfo.scl.z = _animInfo.m_Scl.m_z;

				_newKeyFrame.animInfo.push_back(_newAnimInfo);
			}
			_newAnim.keyFrameInfo.push_back(_newKeyFrame);
		}
		m_pFBXPrefab->m_AnimList.push_back(_newAnim);
	}

	return true;
}

bool FBXLibraryInternal::MultipleAnimationBinaryData(PrefabData preFab)
{
	static int _count = 0; // 이미 한번 들어왔다면 mesh 정보는 있다는 것. 다음부터는 안 넣어줘도 된다.

	if (_count == 0)
	{
		m_pFBXPrefab->name = StringHelper::StringToWString(preFab.m_PrefabName);

		if (m_pFBXPrefab->name.size() == 0) return false; // 파일이 없으면 false 반환

		for (auto _material : preFab.m_MaterialInfoList)
		{
			TL_FBXLibrary::FBXMaterialInfo _newMaterial;
			_newMaterial.materialIndex = _material.m_MaterialIndex;
			_newMaterial.materialName = StringHelper::StringToWString(_material.m_MaterialName);

			for (int i = 0; i < 4; ++i)
			{
				_newMaterial.ambient[i] = _material.m_Ambient[i];
				_newMaterial.diffuse[i] = _material.m_Diffuse[i];
				_newMaterial.specular[i] = _material.m_Specular[i];
				_newMaterial.emissive[i] = _material.m_Emissive[i];
			}

			_newMaterial.roughness = _material.m_Roughness;
			_newMaterial.shininess = _material.m_Shininess;
			_newMaterial.transparency = _material.m_Transparency;
			_newMaterial.reflectivity = _material.m_Reflectivity;

			_newMaterial.baseColorFile = StringHelper::StringToWString(_material.m_BaseColorFile);
			_newMaterial.normalMapFile = StringHelper::StringToWString(_material.m_NormalMapFile);
			_newMaterial.roughnessMapFile = StringHelper::StringToWString(_material.m_RoughnessMapFile);
			_newMaterial.emissiveFile = StringHelper::StringToWString(_material.m_EmissiveFile);
			_newMaterial.ambientFile = StringHelper::StringToWString(_material.m_AmbientFile);
			_newMaterial.specularFile = StringHelper::StringToWString(_material.m_SpecularFile);

			m_pFBXPrefab->m_MaterialList.push_back(_newMaterial);
		}

		for (auto _mesh : preFab.m_MeshDataList)
		{
			TL_FBXLibrary::MeshInfo _newMesh;

			_newMesh.meshNodeName = StringHelper::StringToWString(_mesh.m_MeshNodeName);
			_newMesh.isMaterial = _mesh.m_IsMaterial;
			_newMesh.isStatic = _mesh.m_IsStatic;
			_newMesh.isAnimation = _mesh.m_IsAnimation;

			_newMesh.materialIndexs = _mesh.m_Materials;

			if (_mesh.m_StaticVertex.size() != 0)
			{
				for (auto _vertex : _mesh.m_StaticVertex)
				{
					TL_FBXLibrary::StaticVertex _newVertex;

					_newVertex.pos.x = _vertex.m_Pos.m_x;
					_newVertex.pos.y = _vertex.m_Pos.m_y;
					_newVertex.pos.z = _vertex.m_Pos.m_z;

					_newVertex.uv.x = _vertex.m_UV.m_x;
					_newVertex.uv.y = _vertex.m_UV.m_y;

					_newVertex.normal.x = _vertex.m_Normal.m_x;
					_newVertex.normal.y = _vertex.m_Normal.m_y;
					_newVertex.normal.z = _vertex.m_Normal.m_z;

					_newVertex.tangent.x = _vertex.m_Tangent.m_x;
					_newVertex.tangent.y = _vertex.m_Tangent.m_y;
					_newVertex.tangent.z = _vertex.m_Tangent.m_z;

					_newVertex.bitangent.x = _vertex.m_BiTangent.m_x;
					_newVertex.bitangent.y = _vertex.m_BiTangent.m_y;
					_newVertex.bitangent.z = _vertex.m_BiTangent.m_z;

					_newMesh.StaticVertex.push_back(_newVertex);
				}
			}
			else
			{
				for (auto _vertex : _mesh.m_SkeletalVertex)
				{
					TL_FBXLibrary::SkeletalVertex _newVertex;

					_newVertex.pos.x = _vertex.m_Pos.m_x;
					_newVertex.pos.y = _vertex.m_Pos.m_y;
					_newVertex.pos.z = _vertex.m_Pos.m_z;

					_newVertex.uv.x = _vertex.m_UV.m_x;
					_newVertex.uv.y = _vertex.m_UV.m_y;

					_newVertex.normal.x = _vertex.m_Normal.m_x;
					_newVertex.normal.y = _vertex.m_Normal.m_y;
					_newVertex.normal.z = _vertex.m_Normal.m_z;

					_newVertex.tangent.x = _vertex.m_Tangent.m_x;
					_newVertex.tangent.y = _vertex.m_Tangent.m_y;
					_newVertex.tangent.z = _vertex.m_Tangent.m_z;

					_newVertex.bitangent.x = _vertex.m_BiTangent.m_x;
					_newVertex.bitangent.y = _vertex.m_BiTangent.m_y;
					_newVertex.bitangent.z = _vertex.m_BiTangent.m_z;

					for (int i = 0; i < 4; ++i)
					{
						_newVertex.index[i] = _vertex.m_Index[i];
					}

					_newVertex.weight1.x = _vertex.m_Weight1.m_x;
					_newVertex.weight1.y = _vertex.m_Weight1.m_y;
					_newVertex.weight1.z = _vertex.m_Weight1.m_z;
					_newVertex.weight1.w = _vertex.m_Weight1.m_w;

					_newMesh.SkeletalVertex.push_back(_newVertex);
				}
			}

			for (auto _buffer : _mesh.m_IndexBuffer)
			{
				std::vector<TL_FBXLibrary::MyFace> _faceBuffer;

				for (auto _face : _buffer.first)
				{
					TL_FBXLibrary::MyFace face;

					face.index[0] = _face.m_x;
					face.index[1] = _face.m_y;
					face.index[2] = _face.m_z;

					_faceBuffer.push_back(face);
				}
				_newMesh.indexBuffer.push_back(std::make_pair(_faceBuffer, _buffer.second));
			}

			m_pFBXPrefab->m_MeshList.push_back(_newMesh);
		}

		for (auto _node : preFab.m_NodeList)
		{
			TL_FBXLibrary::FBXNodeInfo _newNode;

			_newNode.nodeName = StringHelper::StringToWString(_node.m_NodeName);
			_newNode.parentNodeName = StringHelper::StringToWString(_node.m_ParentNodeName);
			_newNode.meshNodeName = StringHelper::StringToWString(_node.m_MeshNodeName);

			_newNode.hasMesh = _node.m_HasMesh;
			_newNode.isNegative = _node.m_IsNegative;

			TL_Math::Matrix _newNodeTM(_node.m_NodeTM.m_11, _node.m_NodeTM.m_12, _node.m_NodeTM.m_13, _node.m_NodeTM.m_14
				, _node.m_NodeTM.m_21, _node.m_NodeTM.m_22, _node.m_NodeTM.m_23, _node.m_NodeTM.m_24
				, _node.m_NodeTM.m_31, _node.m_NodeTM.m_32, _node.m_NodeTM.m_33, _node.m_NodeTM.m_34
				, _node.m_NodeTM.m_41, _node.m_NodeTM.m_42, _node.m_NodeTM.m_43, _node.m_NodeTM.m_44);
			_newNode.nodeTM = _newNodeTM;

			m_pFBXPrefab->m_ObjNodeList.push_back(_newNode);
		}

		for (auto _bone : preFab.m_BoneList)
		{
			TL_FBXLibrary::MySkeleton _newBone;

			_newBone.boneName = StringHelper::StringToWString(_bone.m_BoneName);
			_newBone.parentboneName = StringHelper::StringToWString(_bone.m_ParentBoneName);

			_newBone.parentBoneIndex = _bone.m_ParentBoneIndex;

			TL_Math::Matrix _newWorldTM(_bone.m_WorldTM.m_11, _bone.m_WorldTM.m_12, _bone.m_WorldTM.m_13, _bone.m_WorldTM.m_14
				, _bone.m_WorldTM.m_21, _bone.m_WorldTM.m_22, _bone.m_WorldTM.m_23, _bone.m_WorldTM.m_24
				, _bone.m_WorldTM.m_31, _bone.m_WorldTM.m_32, _bone.m_WorldTM.m_33, _bone.m_WorldTM.m_34
				, _bone.m_WorldTM.m_41, _bone.m_WorldTM.m_42, _bone.m_WorldTM.m_43, _bone.m_WorldTM.m_44);
			_newBone.m_WorldTM = _newWorldTM;

			TL_Math::Matrix _newOffsetTM(_bone.m_OffestTM.m_11, _bone.m_OffestTM.m_12, _bone.m_OffestTM.m_13, _bone.m_OffestTM.m_14
				, _bone.m_OffestTM.m_21, _bone.m_OffestTM.m_22, _bone.m_OffestTM.m_23, _bone.m_OffestTM.m_24
				, _bone.m_OffestTM.m_31, _bone.m_OffestTM.m_32, _bone.m_OffestTM.m_33, _bone.m_OffestTM.m_34
				, _bone.m_OffestTM.m_41, _bone.m_OffestTM.m_42, _bone.m_OffestTM.m_43, _bone.m_OffestTM.m_44);

			_newBone.m_bone_From3DSceneSpaceToBoneSpaceTM = _newOffsetTM;

			m_pFBXPrefab->m_BoneList.push_back(_newBone);
		}

		++_count; // mesh 데이터를 넣어주었다. 
	}

	// 애니메이션 데이터는 파일마다 다르기 때문에 각각 넣어주자. 
	for (auto _anim : preFab.m_AnimList)
	{
		TL_FBXLibrary::Animation _newAnim;

		_newAnim.name = StringHelper::StringToWString(_anim.m_Name);
		_newAnim.typeName = StringHelper::StringToWString(_anim.m_TypeName);

		_newAnim.frameRate = _anim.m_FrameRate;
		_newAnim.tickPerFrame = _anim.m_TickPerFrame;
		_newAnim.totalKeyFrame = _anim.m_TotalKeyFrame;
		_newAnim.startKeyFrame = _anim.m_StartKeyFrame;
		_newAnim.endKeyFrame = _anim.m_EndKeyFrame;

		for (auto _keyFrame : _anim.m_KeyFrameInfo)
		{
			TL_FBXLibrary::keyFrameInfo _newKeyFrame;

			_newKeyFrame.nodeName = StringHelper::StringToWString(_keyFrame.m_NodeName);
			_newKeyFrame.boneName = StringHelper::StringToWString(_keyFrame.m_BoneName);

			for (auto _animInfo : _keyFrame.m_AnimData)
			{
				TL_FBXLibrary::AnimationInfo _newAnimInfo;

				_newAnimInfo.time = _animInfo.m_Time;

				_newAnimInfo.pos.x = _animInfo.m_Pos.m_x;
				_newAnimInfo.pos.y = _animInfo.m_Pos.m_y;
				_newAnimInfo.pos.z = _animInfo.m_Pos.m_z;

				_newAnimInfo.rot.x = _animInfo.m_Rot.m_x;
				_newAnimInfo.rot.y = _animInfo.m_Rot.m_y;
				_newAnimInfo.rot.z = _animInfo.m_Rot.m_z;
				_newAnimInfo.rot.w = _animInfo.m_Rot.m_w;

				_newAnimInfo.scl.x = _animInfo.m_Scl.m_x;
				_newAnimInfo.scl.y = _animInfo.m_Scl.m_y;
				_newAnimInfo.scl.z = _animInfo.m_Scl.m_z;

				_newKeyFrame.animInfo.push_back(_newAnimInfo);
			}
			_newAnim.keyFrameInfo.push_back(_newKeyFrame);
		}
		m_pFBXPrefab->m_AnimList.push_back(_newAnim);
	}

	return true;
}