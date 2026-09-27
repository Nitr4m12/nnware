#pragma once

#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_EffectAux.h>
#include <nn/atk/atk_EffectBase.h>
#include <nn/atk/atk_OutputReceiver.h>
#include <nn/atk/fnd/os/atkfnd_CriticalSection.h>

namespace nn::atk {

class OutputMixer : protected OutputReceiver {
public:
    using EffectList =
        util::IntrusiveList<EffectBase,
                            util::IntrusiveListMemberNodeTraits<EffectBase, &EffectBase::m_Link>>;
    using EffectAuxList = util::IntrusiveList<
        EffectAux, util::IntrusiveListMemberNodeTraits<EffectAux, &EffectAux::m_AuxLinkNode>>;

    OutputMixer();

    static size_t GetRequiredMemorySize(int32_t bus, bool isEffectEnabled);

    void Initialize(int32_t bus, bool isEffectEnabled, void* buffer, size_t bufferSize);
    void Finalize();

    bool HasEffect(int32_t bus) const;

    bool AppendEffect(EffectBase* pEffect, int32_t bus, void* buffer, size_t bufferSize);
    bool AppendEffect(EffectAux* pEffect, int32_t bus, void* buffer, size_t bufferSize);

    bool RemoveEffect(EffectBase* pEffect, int32_t bus);
    bool RemoveEffect(EffectAux* pEffect, int32_t bus);

    void ClearEffect(int32_t bus);

    void UpdateEffectAux();

    void OnChangeOutputMode();

    virtual void AppendEffectImpl(EffectBase* pEffect, int32_t bus, void* buffer,
                                  size_t bufferSize);
    virtual void AppendEffectImpl(EffectAux* pEffect, int32_t bus, void* buffer, size_t bufferSize);

    void RemoveEffectImpl(EffectBase* pEffect, int32_t bus);
    void RemoveEffectImpl(EffectAux* pEffect, int32_t bus);

    void ClearEffectImpl(int32_t bus);

private:
    detail::fnd::CriticalSection m_EffectListLock;
    EffectList* m_pEffectList;
    EffectAuxList* m_pEffectAuxList;
    bool m_IsEffectEnabled;
};
static_assert(sizeof(OutputMixer) == 0x40);

}  // namespace nn::atk
