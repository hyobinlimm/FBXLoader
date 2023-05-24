 #pragma once

#include "FBXLibrary/dll.h"
#include "Common.h"
#include <vector>

namespace TL_FBXLibrary
{
	struct FBXNodeInfo;
	struct FBXPrefab;
}

class FBXLibraryInternal;

namespace TL_FBXLibrary
{
	class FBX_LIBRARY_API FBXModelLoader
	{
	public:
		FBXModelLoader();
		~FBXModelLoader();

		// 로드 함수들
		bool Init();
		bool Release();

		bool Load(const tstring filename); 
		bool AllLoad(); // 경로에 있는 모든 파일을 읽어오는 함수
		void FbxSerialize(const tstring filename);
		bool BinaryLoad(tstring filename);
		bool BinaryFolderLoad(tstring filename);

		void FbxConvertOptimize();

		std::vector<TL_FBXLibrary::FBXNodeInfo> GetMeshList();

		int GetObjMeshListSize();
		
		TL_FBXLibrary::FBXPrefab* GetPrefab();

		void TextSerialize(); 
		void BinarySerialize();

	private:
		FBXLibraryInternal* m_FBXLoader;
	};
}


