
#include "FBXLibrary/FBXModelLoader.h"

#include "FBXLibraryInternal.h"
#include "FBXLibrary/FBXParserData.h"



TL_FBXLibrary::FBXModelLoader::FBXModelLoader()
	:m_FBXLoader(nullptr)
{
}

TL_FBXLibrary::FBXModelLoader::~FBXModelLoader()
{
	if (m_FBXLoader != nullptr)
	{
		/*
		* Init() 에서 생성 하고 있는거 삭제 추가함
		* m_FBXLoader = new FBXLibraryInternal();
		*/
		delete m_FBXLoader;
		m_FBXLoader = nullptr;
	}
}

bool TL_FBXLibrary::FBXModelLoader::Init()
{
	m_FBXLoader = new FBXLibraryInternal();
	bool _result = m_FBXLoader->Init();

	return _result;
}

bool TL_FBXLibrary::FBXModelLoader::Release()
{
	bool _result = m_FBXLoader->Release();

	return _result;
}

bool TL_FBXLibrary::FBXModelLoader::Load(const tstring filename)
{
	bool _result = m_FBXLoader->Load(filename);

	// 가져온 data를 최적화

	return _result;
}

bool TL_FBXLibrary::FBXModelLoader::AllLoad()
{
	/// TODO : 구현 해 놓자.
	m_FBXLoader->FilesLoad();

	return  true;
}

void TL_FBXLibrary::FBXModelLoader::FbxSerialize(const tstring filename)
{
	m_FBXLoader = new FBXLibraryInternal();

	m_FBXLoader->FBXSerialization(filename);
}

bool TL_FBXLibrary::FBXModelLoader::BinaryLoad(tstring filename)
{
	m_FBXLoader = new FBXLibraryInternal();

	bool _isFile = false; 
	std::string _fileName = StringHelper::WStringToString(filename);
	int _count1 = _fileName.rfind(".TL");
	int _count2 = _fileName.rfind(".fbx");

	if (_count1 != -1 || _count2 != -1) { _isFile = true; }

	bool _result; // 결과 반환 변수 
	if(_isFile == true)
	{
		_result  = m_FBXLoader->BinaryLoad(filename);
	}
	else
	{
		_result = m_FBXLoader->BinaryFolderLoad(filename);
	}
	

	return _result;
}

bool TL_FBXLibrary::FBXModelLoader::BinaryFolderLoad(tstring filename)
{

	m_FBXLoader = new FBXLibraryInternal();

	const bool _result = m_FBXLoader->BinaryFolderLoad(filename);

	return _result;
}

void TL_FBXLibrary::FBXModelLoader::FbxConvertOptimize()
{
	m_FBXLoader->AllConvertOptimize();
}

std::vector<TL_FBXLibrary::FBXNodeInfo> TL_FBXLibrary::FBXModelLoader::GetMeshList()
{
	return m_FBXLoader->m_pFBXPrefab->m_ObjNodeList;
}

int TL_FBXLibrary::FBXModelLoader::GetObjMeshListSize()
{
	return m_FBXLoader->GetObjMeshListSize();
}

TL_FBXLibrary::FBXPrefab* TL_FBXLibrary::FBXModelLoader::GetPrefab()
{
	return m_FBXLoader->m_pFBXPrefab.get();
}

void TL_FBXLibrary::FBXModelLoader::TextSerialize()
{
	m_FBXLoader->TextSerialize();
}

void TL_FBXLibrary::FBXModelLoader::BinarySerialize()
{
	m_FBXLoader->BinarySerialize();
}
