#pragma once

#define DECLARE_SINGLETON_CLASS(TClass) \
    public: \
        static TClass* GetInstance() { return TClass::Instance; } \
        TClass(); \
        virtual ~TClass(); \
    protected: \
        static TClass* Instance;

#define DEFINE_SINGLETON_CLASS(TClass, ConstructorFunc, DestructorFunc) \
        TClass* TClass::Instance = nullptr; \
        TClass::TClass() { \
            assert(TClass::Instance == nullptr); \
            TClass::Instance = this; \
            std::function<void(void)> func = ConstructorFunc; \
            func(); \
        } \
        TClass::~TClass() { \
            assert(TClass::Instance == this); \
            TClass::Instance = nullptr; \
            std::function<void(void)> func = DestructorFunc; \
            func(); \
        }

