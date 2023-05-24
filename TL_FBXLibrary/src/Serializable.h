#pragma once

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/utility.hpp>

class NodeData;
class MaterialData;
class MeshData;
class MySkeleton;
class Animation;

struct MyFace
{
public:
	MyFace() = default;
	MyFace(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_x;
		ar& m_y;
		ar& m_z;
	}

	unsigned int m_x;
	unsigned int m_y;
	unsigned int m_z;
};

class Float2
{
public:
	Float2() = default;
	Float2(float x, float y) : m_x(x), m_y(y) {}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_x;
		ar& m_y;
	}

	float m_x;
	float m_y;
};

class Float3
{
public:
	Float3() = default;
	Float3(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_x;
		ar& m_y;
		ar& m_z;
	}

	float m_x;
	float m_y;
	float m_z;
};

class Float4
{
public:
	Float4() = default;
	Float4(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_x;
		ar& m_y;
		ar& m_z;
		ar& m_w;
	}

	float m_x;
	float m_y;
	float m_z;
	float m_w;
};

class Float4X4
{
public:
	Float4X4() = default;
	Float4X4(float _11, float _12, float _13, float _14
		, float _21, float _22, float _23, float _24
		, float _31, float _32, float _33, float _34
		, float _41, float _42, float _43, float _44)
		: m_11(_11), m_12(_12), m_13(_13), m_14(_14)
		, m_21(_21), m_22(_22), m_23(_23), m_24(_24)
		, m_31(_31), m_32(_32), m_33(_33), m_34(_34)
		, m_41(_41), m_42(_42), m_43(_43), m_44(_44) {}

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_11;	ar& m_12;	ar& m_13;	ar& m_14;
		ar& m_21;	ar& m_22;	ar& m_23;	ar& m_24;
		ar& m_31;	ar& m_32;	ar& m_33;	ar& m_34;
		ar& m_41;	ar& m_42;	ar& m_43;	ar& m_44;
	}

	float m_11, m_12, m_13, m_14;
	float m_21, m_22, m_23, m_24;
	float m_31, m_32, m_33, m_34;
	float m_41, m_42, m_43, m_44;
};

class PrefabData
{
private:
	friend class boost::serialization::access;

public:
	PrefabData() = default;
	PrefabData(std::string name) : m_PrefabName(std::move(name))
	{};

private:
	template<class Archive>
	void serialize(Archive& ar,
		const unsigned int version)
	{
		ar& m_PrefabName;
		ar& m_MaterialInfoList;
		ar& m_MeshDataList;
		ar& m_NodeList;
		ar& m_BoneList;
		ar& m_AnimList;
	}

public:
	std::string m_PrefabName;

	std::vector<MaterialData> m_MaterialInfoList;
	std::vector<MeshData> m_MeshDataList;
	std::vector<NodeData> m_NodeList;
	std::vector<MySkeleton> m_BoneList;
	std::vector<Animation> m_AnimList;
};

class MaterialData
{
private:
	friend class boost::serialization::access;

public:
	MaterialData() = default;
	MaterialData(int index, std::string name
		, const std::array<float, 4>& ambient
		, const std::array<float, 4>& diffuse
		, const std::array<float, 4>& specular
		, const std::array<float, 4>& emissive
		, const float roughness
		, const float shininess
		, const float transparency
		, const float reflectivity
		, std::string baseColorFile
		, std::string normalMapFile
		, std::string roughnessMapFile
		, std::string EmissiveFile
		, std::string ambientFile
		, std::string specularFile
	)
		: m_MaterialIndex(index)
		, m_MaterialName(std::move(name))
		, m_Ambient{ ambient.at(0), ambient.at(1), ambient.at(2), ambient.at(3) }
		, m_Diffuse{ diffuse.at(0), diffuse.at(1), diffuse.at(2), diffuse.at(3) }
		, m_Specular{ specular.at(0), specular.at(1), specular.at(2), specular.at(3) }
		, m_Emissive{ emissive.at(0), emissive.at(1), emissive.at(2), emissive.at(3) }
		, m_Roughness(roughness)
		, m_Shininess(shininess)
		, m_Transparency(transparency)
		, m_Reflectivity(reflectivity)
		, m_BaseColorFile(std::move(baseColorFile))
		, m_NormalMapFile(std::move(normalMapFile))
		, m_RoughnessMapFile(std::move(roughnessMapFile))
		, m_EmissiveFile(std::move(EmissiveFile))
		, m_AmbientFile(std::move(ambientFile))
		, m_SpecularFile(std::move(specularFile)) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_MaterialIndex;
		ar& m_MaterialName;

		ar& m_Ambient;
		ar& m_Diffuse;
		ar& m_Specular;
		ar& m_Emissive;

		ar& m_Roughness;
		ar& m_Shininess;
		ar& m_Transparency;
		ar& m_Reflectivity;

		ar& m_BaseColorFile;
		ar& m_NormalMapFile;
		ar& m_RoughnessMapFile;
		ar& m_EmissiveFile;
		ar& m_AmbientFile;
		ar& m_SpecularFile;
	}

public:
	int m_MaterialIndex;

	std::string m_MaterialName;

	float m_Ambient[4];
	float m_Diffuse[4];
	float m_Specular[4];
	float m_Emissive[4];

	float m_Roughness;
	float m_Shininess;
	float m_Transparency;
	float m_Reflectivity;

	std::string m_BaseColorFile;
	std::string m_NormalMapFile;
	std::string m_RoughnessMapFile;
	std::string m_EmissiveFile;
	std::string m_AmbientFile;
	std::string m_SpecularFile;
};

class StaticVertex
{
private:
	friend class boost::serialization::access;

public:
	StaticVertex() = default;
	StaticVertex(Float3 pos, Float2 uv, Float3 normal, Float3 tangent, Float3 bitangent)
		:m_Pos(pos)
		, m_UV(uv)
		, m_Normal(normal)
		, m_Tangent(tangent)
		, m_BiTangent(bitangent) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_Pos;
		ar& m_UV;
		ar& m_Normal;
		ar& m_Tangent;
		ar& m_BiTangent;
	}

public:
	Float3 m_Pos; 			// 좌표상의 위치값
	Float2 m_UV;
	Float3 m_Normal;		// 노말값
	Float3 m_Tangent;		// 노말값
	Float3 m_BiTangent;		// 노말값
};

class SkeletalVertex
{
private:
	friend class boost::serialization::access;

public:
	SkeletalVertex() = default;
	SkeletalVertex(Float3 pos
		, const Float2 uv
		, const Float3 normal
		, const Float3 tangent
		, const Float3 bitangent
		, const std::array<int, 4> index
		, const Float4 weight)
		: m_Pos(pos)
		, m_UV(uv)
		, m_Normal(normal)
		, m_Tangent(tangent)
		, m_BiTangent(bitangent)
		, m_Index{ index[0], index[1], index[2], index[3] }
		, m_Weight1(weight) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_Pos;
		ar& m_UV;
		ar& m_Normal;
		ar& m_Tangent;
		ar& m_BiTangent;
		ar& m_Index;
		ar& m_Weight1;
	}

public:
	Float3 m_Pos;
	Float2 m_UV;
	Float3 m_Normal;
	Float3 m_Tangent;		// data 없음. 
	Float3 m_BiTangent;		// data 없음.
	int m_Index[4];
	Float4 m_Weight1;
};

class MeshData
{
private:
	friend class boost::serialization::access;

public:
	MeshData() = default;
	MeshData(std::string name, bool isMaterial, bool isStatic, bool isAnimation)
		: m_MeshNodeName(std::move(name))
		, m_IsMaterial(isMaterial)
		, m_IsStatic(isStatic)
		, m_IsAnimation(isAnimation) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_MeshNodeName;
		ar& m_IsMaterial;
		ar& m_IsStatic;
		ar& m_IsAnimation;

		ar& m_Materials;
		ar& m_StaticVertex;
		ar& m_SkeletalVertex;
		ar& m_IndexBuffer;
	}

public:
	std::string m_MeshNodeName;

	bool m_IsMaterial = false;
	bool m_IsStatic = false;
	bool m_IsAnimation = false;

	std::vector<unsigned int> m_Materials;

	std::vector<StaticVertex> m_StaticVertex;
	std::vector<SkeletalVertex> m_SkeletalVertex;

	std::vector<std::pair<std::vector<MyFace>, unsigned int>> m_IndexBuffer;
};

class MySkeleton
{
private:
	friend class boost::serialization::access;

public:
	MySkeleton() = default;
	MySkeleton(std::string name, std::string parentName, int parentIndex, Float4X4 worldTM, Float4X4 offsetTM)
		: m_BoneName(std::move(name))
		, m_ParentBoneName(std::move(parentName))
		, m_ParentBoneIndex(parentIndex)
		, m_WorldTM(worldTM)
		, m_OffestTM(offsetTM) {};

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_BoneName;
		ar& m_ParentBoneName;

		ar& m_ParentBoneIndex;

		ar& m_WorldTM;
		ar& m_OffestTM;
	}

public:
	std::string m_BoneName;
	std::string m_ParentBoneName;

	int m_ParentBoneIndex;

	Float4X4 m_WorldTM;
	Float4X4 m_OffestTM;
};

class NodeData
{
private:
	friend class boost::serialization::access;

public:
	NodeData() = default;
	NodeData(std::string nodeName, std::string parentName, std::string meshName, bool hasMesh, bool isNegative, Float4X4 nodeTM )
		: m_NodeName(std::move(nodeName))
		, m_ParentNodeName(std::move(parentName))
		, m_MeshNodeName(std::move(meshName))
		, m_HasMesh(hasMesh)
		, m_IsNegative(isNegative)
		, m_NodeTM(nodeTM) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_NodeName;
		ar& m_ParentNodeName;
		ar& m_MeshNodeName;
		ar& m_HasMesh;
		ar& m_IsNegative;
		ar& m_NodeTM; 
	}

public:
	std::string m_NodeName;
	std::string m_ParentNodeName;
	std::string m_MeshNodeName; // MeshNode의 이름

	bool m_HasMesh; // Mesh Node 여부
	bool m_IsNegative; // Negative scale 여부

	Float4X4 m_NodeTM;
};

class AnimationData
{
private:
	friend class boost::serialization::access;

public:
	AnimationData() = default;
	AnimationData(float time, Float3 pos, Float4 rot, Float3 scl)
		:m_Time(time)
		, m_Pos(pos)
		, m_Rot(rot)
		, m_Scl(scl) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_Time;
		ar& m_Pos;
		ar& m_Rot;
		ar& m_Scl;
	}

public:
	float m_Time;

	Float3 m_Pos;
	Float4 m_Rot;
	Float3 m_Scl;
};

class KeyFrameInfo
{
private:
	friend class boost::serialization::access;

public:
	KeyFrameInfo() = default;
	KeyFrameInfo(std::string nodeName, std::string boneName)
		:m_NodeName(std::move(nodeName))
		, m_BoneName(std::move(boneName)) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_NodeName;
		ar& m_BoneName;
		ar& m_AnimData;
	}

public:
	std::string m_NodeName;
	std::string m_BoneName;

	std::vector<AnimationData> m_AnimData;
};

class Animation
{
private:
	friend class boost::serialization::access;

public:
	Animation() = default;
	Animation(std::string name, std::string typeNme, float frameRate, int tickPerFrame, int totalKeyFrame, int startKeyFrame, int endKeyFrame)
		:m_Name(std::move(name))
		, m_TypeName(std::move(typeNme))
		, m_FrameRate(frameRate)
		, m_TickPerFrame(tickPerFrame)
		, m_TotalKeyFrame(totalKeyFrame)
		, m_StartKeyFrame(startKeyFrame)
		, m_EndKeyFrame(endKeyFrame) {}

private:
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& m_Name;
		ar& m_TypeName;
		ar& m_FrameRate;
		ar& m_TickPerFrame;
		ar& m_TotalKeyFrame;
		ar& m_StartKeyFrame;
		ar& m_EndKeyFrame;

		ar& m_KeyFrameInfo;
	}

public:
	std::string m_Name;
	std::string m_TypeName; // 사용 할지 안할지 모름

	float m_FrameRate; // 엔진에서 60프레임으로 돌린다고 했을때, 한 프레임을 얼만큼 보여줄건지에 대한 비율
	int m_TickPerFrame;
	int m_TotalKeyFrame;
	int m_StartKeyFrame;
	int m_EndKeyFrame;

	std::vector<KeyFrameInfo> m_KeyFrameInfo;
};