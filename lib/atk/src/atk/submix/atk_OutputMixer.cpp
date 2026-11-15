#include <nn/util.h>
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
#include <nn/atk/atk_OutputMixer.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>
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

bool OutputMixer::ClearEffect(int32_t bus) {
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandEffect>()};
    command->id = detail::DriverCommandId_ClearEffect;
    command->bus = bus;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    command->pOutputMixer = this;
#endif

    cmdmgr.PushCommand(command);
    uint32_t tag{cmdmgr.FlushCommand(true)};
    cmdmgr.WaitCommandReply(tag);

    return true;
}

void OutputMixer::UpdateEffectAux() {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};

    const int32_t busCount{GetBusCount()};
    for (int bus{0}; bus < busCount; ++bus) {
        for (auto itr{m_pEffectAuxList[bus].begin()}; itr != m_pEffectAuxList[bus].end(); ++itr)
            itr->Update();
    }
}

void OutputMixer::OnChangeOutputMode() {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};

    const int32_t busCount{GetBusCount()};
    for (int bus{0}; bus < busCount; ++bus) {
        for (auto itr{m_pEffectAuxList[bus].begin()}; itr != m_pEffectAuxList[bus].end(); ++itr)
            itr->OnChangeOutputMode();
    }
}

void OutputMixer::AppendEffectImpl(EffectBase* pEffect, int32_t bus, void* buffer,
                                   size_t bufferSize) {
    const audio::AudioRendererParameter& param{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererParameter(),
    };

    if (param.sampleRate != 32000 || pEffect->GetSampleRate() != EffectBase::SampleRate_32000) {
        if (param.sampleRate != 48000 || pEffect->GetSampleRate() != EffectBase::SampleRate_48000) {
            return;
        }
    }

    pEffect->SetEffectBuffer(buffer, bufferSize);

    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };
    const bool result{pEffect->AddEffect(&config, this)};
    if (!result)
        return;

    const int channelCount{GetChannelCount()};
    const int channelBase{channelCount * bus};
    int8_t inputOutputChannel[ChannelIndex_Count];
    for (int i{0}; i < channelCount; ++i)
        inputOutputChannel[i] = channelBase + i;

    pEffect->SetEffectInputOutput(inputOutputChannel, inputOutputChannel, channelCount,
                                  channelCount);

    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};
    m_pEffectList[bus].push_back(*pEffect);
}

void OutputMixer::AppendEffectImpl(EffectAux* pEffect, int32_t bus, void* buffer,
                                   size_t bufferSize) {
    if (!pEffect->Initialize())
        return;

    pEffect->SetEffectBuffer(buffer, bufferSize);

    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };
    const audio::AudioRendererParameter& param{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererParameter(),
    };
    const bool result{pEffect->AddEffect(&config, param, this)};
    if (!result)
        return;

    const int channelCount{GetChannelCount()};
    const int channelBase{channelCount * bus};
    int8_t inputOutputChannel[ChannelIndex_Count];
    for (int i{0}; i < channelCount; ++i)
        inputOutputChannel[i] = channelBase + i;

    pEffect->SetEffectInputOutput(inputOutputChannel, inputOutputChannel, channelCount,
                                  channelCount);

    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};
    m_pEffectAuxList[bus].push_back(*pEffect);
}

void OutputMixer::RemoveEffectImpl(EffectBase* pEffect, int32_t bus) {
    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };

    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};

    for (auto itr{m_pEffectList[bus].begin()}; itr != m_pEffectList[bus].end(); ++itr) {
        if (&*itr == pEffect) {
            itr->RemoveEffect(&config, this);
            m_pEffectList[bus].erase(itr);
            AddReferenceCount(-1);
            return;
        }
    }
}

void OutputMixer::RemoveEffectImpl(EffectAux* pEffect, int32_t bus) {
    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };

    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};

    for (auto itr{m_pEffectAuxList[bus].begin()}; itr != m_pEffectAuxList[bus].end(); ++itr) {
        if (&*itr == pEffect) {
            itr->RemoveEffect(&config, this);
            itr->Finalize();
            m_pEffectAuxList[bus].erase(itr);
            AddReferenceCount(-1);
            return;
        }
    }
}

void OutputMixer::ClearEffectImpl(int32_t bus) {
    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_EffectListLock};
    int count{0};

    for (auto itr{m_pEffectList[bus].begin()}; itr != m_pEffectList[bus].end(); ++itr) {
        itr->RemoveEffect(&config, this);
        ++count;
    }
    m_pEffectList[bus].clear();

    for (auto itr{m_pEffectAuxList[bus].begin()}; itr != m_pEffectAuxList[bus].end(); ++itr) {
        itr->RemoveEffect(&config, this);
        itr->Finalize();
        ++count;
    }
    m_pEffectAuxList[bus].clear();
    AddReferenceCount(-count);
}

}  // namespace nn::atk
#endif