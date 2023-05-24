#include <stdio.h>
#include <Common.h>


#include "FBXLibrary/FBXModelLoader.h"
#include "FBXLibrary/FBXParserData.h"


int main()
{
	TL_FBXLibrary::FBXModelLoader* m_FBXLoader = new TL_FBXLibrary::FBXModelLoader();

	// Mesh는 같고, 애니메이션만 다른 .TL가 있다면 폴더 경로를 넣어주세요.
	// 아니라면 그냥 .TL 파일을 넣어주세요.
	//m_FBXLoader->BinaryLoad(TEXT("../../../../../4_Resources/_DevelopmentAssets/Model/Joy"));

	//bool _result;
	//_result = m_FBXLoader->Init();

	m_FBXLoader->FbxSerialize(TEXT("../../../../../4_Resources/_DevelopmentAssets/Model/Player/Player_Idle.fbx"));
	m_FBXLoader->FbxSerialize(TEXT("../../../../../4_Resources/_DevelopmentAssets/Model/Player/Player_Jump.fbx"));
	m_FBXLoader->FbxSerialize(TEXT("../../../../../4_Resources/_DevelopmentAssets/Model/Player/Player_Run.fbx"));

	//_result = m_FBXLoader->AllLoad(); // 모든 파일 시리얼라이즈 하는 함수
	
	//_result = m_FBXLoader->Load(TEXT("_DevelopmentAssets/Model/Player/Player_run.fbx"));
	//_result = m_FBXLoader->Load(TEXT("_DevelopmentAssets/Model/Chapter1_1Stage/Chapter1_1Stage.fbx"));
	//_result = m_FBXLoader->Load(TEXT("_DevelopmentAssets/Model/Joyful_Jump.fbx"));

	//m_FBXLoader->FbxConvertOptimize(); // data 최적화 시키기(아직 최적화 안됨.), 최적화 되면 load 함수 안으로 집어넣을 것. 

	m_FBXLoader->Release();
	//_result = m_FBXLoader->Release();

	return 0;
}

