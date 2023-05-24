#pragma once

#include <functional>
#include <map>

#include "Common/Common_typedef.h"

namespace TL_Common
{
    /**
     * @brief 여러 타입의 Map을 포함하는 Map 클래스입니다.
     */
    class VariousTypeMap
    {
        using TypeKey = unsigned long long;
        using DataStructure = std::map<tstring, void*>;

    public:
        VariousTypeMap() = default;

        ~VariousTypeMap() = default;

    private:
        std::map<TypeKey, DataStructure> m_DataTable;

    public:
        template <typename T>
        void Add(T* _data, const tstring& _keyName);

        template <typename T>
        T* Get(const tstring& _keyName);

        template <typename T>
        bool TryGet(const tstring& _keyName, T** _out);

        template <typename T>
        T* operator[](const tstring& _keyName);

        template <typename T>
        void Delete(const tstring& _keyName);

        void Clear();

    private:
        template <typename T>
        TypeKey GetTypeKey();
    };

    inline void VariousTypeMap::Clear()
    {
        m_DataTable.clear();
    }

    template <typename T>
    void VariousTypeMap::Add(T* _data, const tstring& _keyName)
    {
        m_DataTable[GetTypeKey<T>()][_keyName] = _data;
    }

    template <typename T>
    inline T* VariousTypeMap::Get(const tstring& _keyName)
    {
        auto result = m_DataTable[GetTypeKey<T>()].find(_keyName);

        if (result == m_DataTable[GetTypeKey<T>()].end())
            throw "No Data";

        return static_cast<T*>(result->second);
    }

    template <typename T>
    bool VariousTypeMap::TryGet(const tstring& _keyName, T** _out)
    {
        auto result = m_DataTable[GetTypeKey<T>()].find(_keyName);

        if (result == m_DataTable[GetTypeKey<T>()].end())
            return false;

        *_out = reinterpret_cast<T*>(result->second.data);
        return true;
    }

    template <typename T>
    T* VariousTypeMap::operator[](const tstring& _keyName)
    {
        return Get<T>(_keyName);
    }

    template <typename T>
    void VariousTypeMap::Delete(const tstring& _keyName)
    {
        auto result = m_DataTable[GetTypeKey<T>()].find(_keyName);

        if (result == m_DataTable[GetTypeKey<T>()].end())
            throw "No Data";

        m_DataTable[GetTypeKey<T>()].erase(result);
    }

    template <typename T>
    VariousTypeMap::TypeKey VariousTypeMap::GetTypeKey()
    {
        static int typeOnlyVariable;
        return (TypeKey)&typeOnlyVariable;
    }
}
