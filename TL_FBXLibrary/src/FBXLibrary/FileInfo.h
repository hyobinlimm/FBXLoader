#pragma once

#include <Common.h>

struct FileInfo
{
private:
	tstring m_Name;
	tstring m_PathMap;
	tstring m_PathASE;
	tstring m_PathFBX;

public:
	tstring GetName() const { return m_Name; }
	void SetName(tstring  val) { m_Name = val; }

	const tstring GetPathMap() const {
		if (m_PathMap == L"")
			return L"Fail";

		tstring _texturePath(L"./Resource/" + m_PathMap + L".dds");
		return _texturePath;
	}

	void SetPathMap(tstring val) { m_PathMap = val; }

	tstring GetPathASE() const {
		tstring _ASEPath(L"./Resource/" + m_PathASE + L".ASE");
		return _ASEPath;
	}

	void SetPathASE(tstring val) { m_PathASE = val; }


	tstring GetPathFBX() const
	{
		tstring _ASEPath(L"./Resource/" + m_PathFBX + L".fbx");
		return _ASEPath;
	}

	void SetPathFBX(tstring val) { m_PathFBX = val; };
};