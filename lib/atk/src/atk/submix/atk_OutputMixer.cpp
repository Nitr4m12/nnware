#include <nn/atk/atk_OutputMixer.h>

#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>
#include "nn/atk/atk_DriverCommand.h"
#include "nn/util.h"
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
#include <nn/atk/fnd/basis/atkfnd_WorkBufferAllocator.h>
#endif

namespace nn::atk {

OutputMixer::OutputMixer() = default;

size_t OutputMixer::GetRequiredMemorySize(int32_t bus, bool isEffectEnabled) {
    size_t result{0};
    if (!isEffectEnabled)
        return 0;

    result = bus;
    result *= 32;

    return result;
}

void OutputMixer::Initialize(int32_t bus, bool isEffectEnabled, void* buffer,
                             [[maybe_unused]] size_t bufferSize) {
    if (isEffectEnabled) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        uint8_t* ptr{static_cast<uint8_t*>(buffer)};
        m_pEffectList = reinterpret_cast<EffectList*>(ptr);
#else
        detail::fnd::WorkBufferAllocator allocator{buffer, bufferSize};
        m_pEffectList = allocator.Allocate<EffectList>(bus);
#endif
        for (int32_t i{0}; i < bus; ++i)
            new (&m_pEffectList[i]) EffectList;

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        ptr += bus * sizeof(EffectList);
        m_pEffectAuxList = reinterpret_cast<EffectAuxList*>(ptr);
#else
        m_pEffectAuxList = allocator.Allocate<EffectAuxList>(bus);
#endif
        for (int32_t i{0}; i < bus; ++i)
            new (&m_pEffectAuxList[i]) EffectList;
    }

    m_IsEffectEnabled = isEffectEnabled;
}

void OutputMixer::Finalize() {
    m_pEffectList = nullptr;
    m_pEffectAuxList = nullptr;
    m_IsEffectEnabled = false;
}

bool OutputMixer::HasEffect(int32_t bus) const {
    if (!m_IsEffectEnabled)
        return false;

    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};
    if (m_pEffectList[bus].empty() && m_pEffectAuxList[bus].empty())
        return false;

    return true;
}

bool OutputMixer::AppendEffect(EffectBase* pEffect, int32_t bus, void* buffer, size_t bufferSize) {
    int32_t effectSampleRate{pEffect->GetSampleRate()};
    switch (effectSampleRate) {
    case EffectBase::SampleRate_32000:
    case EffectBase::SampleRate_48000:
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }

    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandEffect>()};
    command->id = detail::DriverCommandId_AppendEffect;
    command->bus = bus;
    command->effect = pEffect;
    command->effectBuffer = buffer;
    command->effectBufferSize = bufferSize;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    command->pOutputMixer = this;
#endif

    cmdmgr.PushCommand(command);
    AddReferenceCount(1);

    return true;
}

bool OutputMixer::AppendEffect(EffectAux* pEffect, int32_t bus, void* buffer, size_t bufferSize) {
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandEffectAux>()};
    command->id = detail::DriverCommandId_AppendEffectAux;
    command->bus = bus;
    command->effect = pEffect;
    command->effectBuffer = buffer;
    command->effectBufferSize = bufferSize;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    command->pOutputMixer = this;
#endif

    cmdmgr.PushCommand(command);
    AddReferenceCount(1);

    return true;
}

bool OutputMixer::RemoveEffect(EffectBase* pEffect, int32_t bus) {
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandEffect>()};
    command->id = detail::DriverCommandId_RemoveEffect;
    command->effect = pEffect;
    command->bus = bus;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    command->pOutputMixer = this;
#endif

    cmdmgr.PushCommand(command);
    uint32_t tag{cmdmgr.FlushCommand(true)};
    cmdmgr.WaitCommandReply(tag);

    return true;
}

bool OutputMixer::RemoveEffect(EffectAux* pEffect, int32_t bus) {
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandEffectAux>()};
    command->id = detail::DriverCommandId_RemoveEffectAux;
    command->effect = pEffect;
    command->bus = bus;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    command->pOutputMixer = this;
#endif

    cmdmgr.PushCommand(command);
    uint32_t tag{cmdmgr.FlushCommand(true)};
    cmdmgr.WaitCommandReply(tag);

    return true;
}

}  // namespace nn::atk
