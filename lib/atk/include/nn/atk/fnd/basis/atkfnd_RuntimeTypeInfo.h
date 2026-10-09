#pragma once

#include <type_traits>

namespace nn::atk::detail::fnd {

class RuntimeTypeInfo {
public:
    explicit RuntimeTypeInfo(const RuntimeTypeInfo* parent) : m_ParentTypeInfo{parent} {}

    bool IsDerivedFrom(const RuntimeTypeInfo* s_TypeInfo) const {
        const RuntimeTypeInfo* self{this};

        if (s_TypeInfo == nullptr)
            return false;

        if (s_TypeInfo == self)
            return true;

        return IsDerivedFrom(s_TypeInfo->m_ParentTypeInfo);
    }

private:
    const RuntimeTypeInfo* m_ParentTypeInfo;
};
static_assert(sizeof(RuntimeTypeInfo) == 0x8);

// XXX: currently, functions calling GetRuntimeTypeInfoStatic will only match when using
// __attribute__((noinline))
#define NN_ATK_RTTI_BASE(CLASS)                                                                    \
public:                                                                                            \
    __attribute__((noinline)) static const nn::atk::detail::fnd::RuntimeTypeInfo*                  \
    GetRuntimeTypeInfoStatic() {                                                                   \
        static const nn::atk::detail::fnd::RuntimeTypeInfo s_TypeInfo{nullptr};                    \
        return &s_TypeInfo;                                                                        \
    }                                                                                              \
                                                                                                   \
    virtual const nn::atk::detail::fnd::RuntimeTypeInfo* GetRuntimeTypeInfo() const {              \
        return CLASS::GetRuntimeTypeInfoStatic();                                                  \
    }

#define NN_ATK_RTTI_OVERRIDE(CLASS, BASE)                                                          \
public:                                                                                            \
    static const nn::atk::detail::fnd::RuntimeTypeInfo* GetRuntimeTypeInfoStatic() {               \
        static const nn::atk::detail::fnd::RuntimeTypeInfo s_TypeInfo{                             \
            BASE::GetRuntimeTypeInfoStatic()};                                                     \
        return &s_TypeInfo;                                                                        \
    }                                                                                              \
                                                                                                   \
    const nn::atk::detail::fnd::RuntimeTypeInfo* GetRuntimeTypeInfo() const override {             \
        return CLASS::GetRuntimeTypeInfoStatic();                                                  \
    }

template <typename TToPtr, typename TFrom>
inline TToPtr DynamicCast(TFrom* obj) {
    const RuntimeTypeInfo* typeInfoU{std::remove_pointer_t<TToPtr>::GetRuntimeTypeInfoStatic()};

    if (obj != nullptr && typeInfoU->IsDerivedFrom(obj->GetRuntimeTypeInfo()))
        return static_cast<TToPtr>(obj);

    return nullptr;
}

}  // namespace nn::atk::detail::fnd
