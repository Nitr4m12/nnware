#include <nn/atk/atk_SubMix.h>

#include <algorithm>

#include <nn/audio.h>
#include <nn/util/util_BitUtil.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
#include <nn/atk/fnd/basis/atkfnd_WorkBufferAllocator.h>
#endif
#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>

namespace nn::atk {

SubMix::SubMixParam::SubMixParam() {
    for (int i{0}; i < BusCountMax; ++i)
        m_IsSoundSendClampEnabledArray[i] = true;
}

void SubMix::SubMixParam::SetSrcBusCount(int32_t count) {
    m_SrcBusCount = count;
}

int32_t SubMix::SubMixParam::GetSrcBusCount() const {
    return m_SrcBusCount;
}

void SubMix::SubMixParam::SetSrcChannelCount(int32_t count) {
    m_SrcChannelCount = count;
}

int32_t SubMix::SubMixParam::GetSrcChannelCount() const {
    return m_SrcChannelCount;
}

void SubMix::SubMixParam::SetDstBusCount(int32_t count) {
    m_DstBusCount = count;
}

int32_t SubMix::SubMixParam::GetDstBusCount() const {
    return m_DstBusCount;
}

void SubMix::SubMixParam::SetDstChannelCount(int32_t count) {
    m_DstChannelCount = count;
}

int32_t SubMix::SubMixParam::GetDstChannelCount() const {
    return m_DstChannelCount;
}

void SubMix::SubMixParam::SetOutputReceiver(const OutputReceiver* pOutputReceiver) {
    m_pOutputReceiver = pOutputReceiver;
}

const OutputReceiver* SubMix::SubMixParam::GetOutputReceiver() const {
    return m_pOutputReceiver;
}

void SubMix::SubMixParam::SetEffectEnabled(bool isEnabled) {
    m_IsEffectEnabled = isEnabled;
}

bool SubMix::SubMixParam::IsEffectEnabled() const {
    return m_IsEffectEnabled;
}

void SubMix::SubMixParam::SetSoundSendClampEnabled(int32_t bus, bool isEnabled) {
    m_IsSoundSendClampEnabledArray[bus] = isEnabled;
}

bool SubMix::SubMixParam::IsSoundSendClampEnabled(int32_t bus) const {
    return m_IsSoundSendClampEnabledArray[bus];
}

SubMix::VolumeData::VolumeData() {
    m_Volume.InitValue(1.0f);
}

bool SubMix::VolumeData::Update() {
    bool result{true};
    result = m_IsDirtyFlag;
    m_IsDirtyFlag = false;

    if (m_Volume.IsFinished()) {
        if (m_IsMute == m_IsPrevMute)
            return result;

        m_IsPrevMute = m_IsMute;
    }

    m_Volume.Update();
    return true;
}

SubMix::SubMix() = default;

size_t SubMix::GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                     int32_t dstBusCount, int32_t dstChannelCount) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return GetRequiredMemorySize(srcBusCount, srcChannelCount, dstBusCount, dstChannelCount, true);
#else
    return GetRequiredMemorySize(srcBusCount, srcChannelCount, dstBusCount, dstChannelCount, true,
                                 false);
#endif
}

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
size_t SubMix::GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                     int32_t dstBusCount, int32_t dstChannelCount,
                                     bool isEffectEnabled)
#else
size_t SubMix::GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                     int32_t dstBusCount, int32_t dstChannelCount,
                                     bool isEffectEnabled, bool isInternalCall)
#endif
{
    SubMixParam param;
    param.SetSrcBusCount(srcBusCount);
    param.SetSrcChannelCount(srcChannelCount);
    param.SetDstBusCount(dstBusCount);
    param.SetDstChannelCount(dstChannelCount);
    param.SetEffectEnabled(isEffectEnabled);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return GetRequiredMemorySizeImpl(param);
#else
    return GetRequiredMemorySizeImpl(param, isInternalCall);
#endif
}

size_t SubMix::GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                     const OutputReceiver* pDstReceiver) {
    return GetRequiredMemorySize(srcBusCount, srcChannelCount, pDstReceiver, true);
}

size_t SubMix::GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                     const OutputReceiver* pDstReceiver, bool isEffectEnabled) {
    SubMixParam param;
    param.SetSrcBusCount(srcBusCount);
    param.SetSrcBusCount(srcBusCount);
    param.SetSrcChannelCount(srcChannelCount);
    param.SetOutputReceiver(pDstReceiver);
    param.SetEffectEnabled(isEffectEnabled);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return GetRequiredMemorySizeImpl(param);
#else
    return GetRequiredMemorySizeImpl(param, false);
#endif
}

size_t SubMix::GetRequiredMemorySize(const SubMixParam& param) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return GetRequiredMemorySizeImpl(param);
#else
    return GetRequiredMemorySizeImpl(param, false);
#endif
}

bool SubMix::Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                        int32_t dstChannelCount, void* buffer, size_t bufferSize) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return Initialize(srcBusCount, srcChannelCount, dstBusCount, dstChannelCount, true, buffer,
                      bufferSize);
#else
    return Initialize(srcBusCount, srcChannelCount, dstBusCount, dstChannelCount, true, false,
                      buffer, bufferSize);
#endif
}

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
bool SubMix::Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                        int32_t dstChannelCount, bool isEffectEnabled, void* buffer,
                        size_t bufferSize)
#else
bool SubMix::Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                        int32_t dstChannelCount, bool isEffectEnabled, bool isInternalCall,
                        void* buffer, size_t bufferSize)
#endif
{
    SubMixParam param;
    param.SetSrcBusCount(srcBusCount);
    param.SetSrcChannelCount(srcChannelCount);
    param.SetDstBusCount(dstBusCount);
    param.SetDstChannelCount(dstChannelCount);
    param.SetEffectEnabled(isEffectEnabled);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return InitializeImpl(param, buffer, bufferSize);
#else
    return InitializeImpl(param, buffer, bufferSize, isInternalCall);
#endif
}

bool SubMix::Initialize(int32_t srcBusCount, int32_t srcChannelCount,
                        const OutputReceiver* pDstReceiver, void* buffer, size_t bufferSize) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return Initialize(srcBusCount, srcChannelCount, pDstReceiver->GetBusCount(),
                      pDstReceiver->GetChannelCount(), true, buffer, bufferSize);
#else
    return Initialize(srcBusCount, srcChannelCount, pDstReceiver->GetBusCount(),
                      pDstReceiver->GetChannelCount(), true, false, buffer, bufferSize);
#endif
}

bool SubMix::Initialize(int32_t srcBusCount, int32_t srcChannelCount,
                        const OutputReceiver* pDstReceiver, bool isEffectEnabled, void* buffer,
                        size_t bufferSize) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return Initialize(srcBusCount, srcChannelCount, pDstReceiver->GetBusCount(),
                      pDstReceiver->GetChannelCount(), isEffectEnabled, buffer, bufferSize);
#else
    return Initialize(srcBusCount, srcChannelCount, pDstReceiver->GetBusCount(),
                      pDstReceiver->GetChannelCount(), isEffectEnabled, false, buffer, bufferSize);
#endif
}

bool SubMix::Initialize(const SubMixParam& param, void* buffer, size_t bufferSize) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    return InitializeImpl(param, buffer, bufferSize);
#else
    return InitializeImpl(param, buffer, bufferSize, false);
#endif
}

void SubMix::Finalize() {
    if (m_IsInitialized) {
        detail::driver::HardwareManager::GetInstance().RemoveSubMix(this);
        m_IsInitialized = false;

        detail::driver::HardwareManager::UpdateAudioRendererScopedLock lock;
        audio::AudioRendererConfig& config{
            detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
        };
        audio::ReleaseSubMix(&config, &m_SubMix);
    }

    if (m_pReceiver != nullptr)
        m_pReceiver->AddReferenceCount(-1);

    m_ppSendVolume = nullptr;
    m_pBusVolume = nullptr;
    m_pChannelVolume = nullptr;
    m_pReceiver = nullptr;

    OutputMixer::Finalize();
}

bool SubMix::IsRemovable() const {
    return IsFinalizable();
}

bool SubMix::IsFinalizable() const {
    return m_ReferenceCount == 0;
}

void SubMix::Update() {
    for (int bus{0}; bus < m_BusCount; ++bus) {
        if (m_pBusVolume[bus].Update())
            UpdateBusMixVolume(bus);
    }

    for (int channel{0}; channel < m_ChannelCount; ++channel) {
        if (m_pChannelVolume[channel].Update())
            UpdateChannelMixVolume(channel);
    }

    if (m_SubMixVolume.Update()) {
        float volume;
        {
            detail::fnd::ScopedLock<detail::fnd::CriticalSection> volumeLock{m_VolumeLock};
            volume = m_SubMixVolume.GetVolume();
        }

        volume = detail::fnd::Clamp(volume, 0.0f, 128.0f);
        audio::SetSubMixVolume(&m_SubMix, volume);
    }
}

void SubMix::UpdateBusMixVolume(int32_t bus) {
    const int32_t dstBusCount{m_pReceiver->GetBusCount()};
    const int32_t dstChannelCount{m_pReceiver->GetChannelCount()};

    for (int channel{0}; channel < m_ChannelCount; ++channel) {
        for (int dstBus{0}; dstBus < dstBusCount; ++dstBus) {
            for (int dstChannel{0}; dstChannel < dstChannelCount; ++dstChannel) {
                UpdateMixVolume(bus, channel, dstBus, dstChannel);
            }
        }
    }
}

void SubMix::UpdateChannelMixVolume(int32_t channel) {
    const int32_t dstBusCount{m_pReceiver->GetBusCount()};
    const int32_t dstChannelCount{m_pReceiver->GetChannelCount()};

    for (int bus{0}; bus < m_BusCount; ++bus) {
        for (int dstBus{0}; dstBus < dstBusCount; ++dstBus) {
            for (int dstChannel{0}; dstChannel < dstChannelCount; ++dstChannel) {
                UpdateMixVolume(bus, channel, dstBus, dstChannel);
            }
        }
    }
}

void SubMix::UpdateMixVolume(int32_t srcBus, int32_t srcChannel, int32_t dstBus,
                             int32_t dstChannel) {
    const int32_t srcIdx{srcChannel + srcBus * m_ChannelCount};
    const int32_t dstIdx{dstChannel + dstBus * m_ReceiverChannelCountMax};
    float volume{0.0f};
    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_VolumeLock};
        volume = m_pBusVolume[srcBus].GetVolume() * m_pChannelVolume[srcChannel].GetVolume() *
                 m_ppSendVolume[srcIdx][dstIdx];
    }
    volume = detail::fnd::Clamp(volume, 0.0f, 128.0f);

    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> destinationLock{m_DestinationLock};
        if (!m_IsAppliedOutputReceiver)
            return;

        switch (m_pReceiver->GetReceiverType()) {
        case ReceiverType::ReceiverType_SubMix:
            audio::SetSubMixMixVolume(&m_SubMix,
                                      static_cast<SubMix*>(m_pReceiver)->GetAudioSubMixInstance(),
                                      volume, srcIdx, dstIdx);
            break;
        case ReceiverType::ReceiverType_FinalMix:
            audio::SetSubMixMixVolume(
                &m_SubMix, static_cast<FinalMix*>(m_pReceiver)->GetAudioFinalMixInstance(), volume,
                srcIdx, dstIdx);
            break;
        default:
            NN_UNEXPECTED_DEFAULT;
        }
    }
}

void SubMix::SetDestination(OutputReceiver* pReceiver) {
    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_DestinationLock};
        m_IsAppliedOutputReceiver = false;
        if (m_pReceiver != nullptr)
            m_pReceiver->AddReferenceCount(-1);

        m_pReceiver = pReceiver;
        m_pReceiver->AddReferenceCount(1);
    }

    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandSubMixApplyDestination>()};
    command->id = detail::DriverCommandId_SubMixApplyDestination;
    command->pReceiver = this;

    cmdmgr.PushCommand(command);
    AddReferenceCount(1);
}

void SubMix::ApplyDestination() {
    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };

    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> destinationLock{m_DestinationLock};
        switch (m_pReceiver->GetReceiverType()) {
        case ReceiverType::ReceiverType_SubMix:
            audio::SetSubMixDestination(
                &config, &m_SubMix, static_cast<SubMix*>(m_pReceiver)->GetAudioSubMixInstance());
            break;
        case ReceiverType::ReceiverType_FinalMix:
            audio::SetSubMixDestination(
                &config, &m_SubMix,
                static_cast<FinalMix*>(m_pReceiver)->GetAudioFinalMixInstance());
            break;
        default:
            NN_UNEXPECTED_DEFAULT;
        }
        m_IsAppliedOutputReceiver = true;
    }

    for (int bus{0}; bus < m_BusCount; ++bus) {
        UpdateBusMixVolume(bus);
    }
}

float SubMix::GetSend(int32_t srcBus, int32_t dstBus) const {
    return GetSendImpl(srcBus, 0, dstBus, 0);
}

void SubMix::SetSend(int32_t srcBus, int32_t dstBus, float send) {
    const int dstChannelCount{m_pReceiver->GetChannelCount()};
    for (int srcChannel{0}; srcChannel < m_ChannelCount; ++srcChannel) {
        for (int dstChannel{0}; dstChannel < dstChannelCount; ++dstChannel) {
            if (srcChannel == dstChannel)
                SetSendImpl(srcBus, srcChannel, dstBus, dstChannel, send);
            else
                SetSendImpl(srcBus, srcChannel, dstBus, dstChannel, 0.0f);
        }
    }
}

void SubMix::SetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel,
                         float volume) {
    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_VolumeLock};
        m_ppSendVolume[srcChannel + srcBus * m_ChannelCount]
                      [dstChannel + dstBus * m_ReceiverChannelCountMax] = volume;
    }
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    auto* command{cmdmgr.AllocCommand<detail::DriverCommandSubMixUpdateMixVolume>()};
    command->id = detail::DriverCommandId_SubMixUpdateMixVolume;
    command->pReceiver = this;
    command->srcBus = srcBus;
    command->srcChannel = srcChannel;
    command->dstBus = dstBus;
    command->dstChannel = dstChannel;

    cmdmgr.PushCommand(command);
    AddReferenceCount(1);
}

float SubMix::GetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus,
                          int32_t dstChannel) const {
    return m_ppSendVolume[srcChannel + srcBus * m_ChannelCount]
                         [dstChannel + dstBus * m_ReceiverChannelCountMax];
}

void SubMix::SetBusVolume(int32_t bus, float volume, int32_t fadeFrame) {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_VolumeLock};
    m_pBusVolume[bus].SetVolume(volume, fadeFrame);
}

float SubMix::GetBusVolume(int32_t bus) const {
    return m_pBusVolume[bus].GetVolume();
}

void SubMix::SetBusMute(int32_t bus, bool isMute) {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_VolumeLock};
    m_pBusVolume[bus].SetMute(isMute);
}

bool SubMix::IsBusMuted(int32_t bus) const {
    return m_pBusVolume[bus].IsMute();
}

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
size_t SubMix::GetRequiredMemorySizeImpl(const SubMixParam& param)
#else
size_t SubMix::GetRequiredMemorySizeImpl(const SubMixParam& param,
                                         [[maybe_unused]] bool isInternalCall)
#endif
{
    size_t result{0};
    const int SrcBusCount{param.GetSrcBusCount()};
    const int SrcChannelCount{param.GetSrcChannelCount()};
    const int DstBusCount{
        param.GetOutputReceiver() == nullptr ? param.GetDstBusCount() :
                                               param.GetOutputReceiver()->GetBusCount(),
    };
    const int DstChannelCount{
        param.GetOutputReceiver() == nullptr ? param.GetDstChannelCount() :
                                               param.GetOutputReceiver()->GetChannelCount(),
    };

    result += OutputMixer::GetRequiredMemorySize(SrcBusCount, param.IsEffectEnabled());
    result = util::align_up(result, SubMixAlignment);
    result += (SrcChannelCount * SrcBusCount) * sizeof(void*);
    result += DstChannelCount * DstBusCount * (SrcChannelCount * SrcBusCount) * sizeof(float);
    result += SrcBusCount;
    result = util::align_up(result, 4);
    result += (static_cast<size_t>(SrcBusCount) + SrcChannelCount) * sizeof(VolumeData);

    return result;
}

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
bool SubMix::InitializeImpl(const SubMixParam& param, void* buffer,
                            [[maybe_unused]] size_t bufferSize)
#else
bool SubMix::InitializeImpl(const SubMixParam& param, void* buffer, size_t bufferSize,
                            [[maybe_unused]] bool isInternalCall)
#endif
{
    const int SrcBusCount{param.GetSrcBusCount()};
    const int SrcChannelCount{param.GetSrcChannelCount()};
    const bool IsOutputReceiverUsed{param.GetOutputReceiver() != nullptr};
    const int DstBusCount{
        IsOutputReceiverUsed ? param.GetOutputReceiver()->GetBusCount() : param.GetDstBusCount(),
    };
    const int DstChannelCount{
        IsOutputReceiverUsed ? param.GetOutputReceiver()->GetChannelCount() :
                               param.GetDstChannelCount(),
    };
    const bool IsEffectEnabled{param.IsEffectEnabled()};

    audio::AudioRendererConfig& config{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
    };
    int32_t sampleRate{
        detail::driver::HardwareManager::GetInstance().GetAudioRendererParameter().sampleRate,
    };
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    detail::fnd::WorkBufferAllocator allocator{buffer, bufferSize};
#endif

    m_BusCount = SrcBusCount;
    m_ChannelCount = SrcChannelCount;
    m_ReceiverChannelCountMax = DstChannelCount;
    m_IsAppliedOutputReceiver = false;

    const size_t requiredSize{OutputMixer::GetRequiredMemorySize(SrcBusCount, IsEffectEnabled)};
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    uint8_t* ptr{static_cast<uint8_t*>(buffer)};
    OutputMixer::Initialize(m_BusCount, IsEffectEnabled, ptr, requiredSize);
    ptr += requiredSize;

    const int mixBufferCount{m_BusCount * m_ChannelCount};
    ptr = util::BytePtr(ptr).AlignUp(SubMixAlignment).Get<uint8_t>();
    m_ppSendVolume = reinterpret_cast<float**>(ptr);
    ptr += mixBufferCount * sizeof(float*);
#else
    OutputMixer::Initialize(m_BusCount, IsEffectEnabled,
                            allocator.Allocate(requiredSize, SubMixAlignment), requiredSize);

    const int mixBufferCount{m_BusCount * m_ChannelCount};

    m_ppSendVolume = allocator.Allocate<float*>(mixBufferCount);
#endif
    for (int srcIdx{0}; srcIdx < mixBufferCount; ++srcIdx) {
        const int32_t receiverMixBufferCount{m_ReceiverChannelCountMax * DstBusCount};
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        m_ppSendVolume[srcIdx] = reinterpret_cast<float*>(ptr);
        ptr += receiverMixBufferCount * sizeof(float);
#else
        m_ppSendVolume[srcIdx] = allocator.Allocate<float>(receiverMixBufferCount);
#endif
        for (int dstIdx{0}; dstIdx < receiverMixBufferCount; ++dstIdx) {
            m_ppSendVolume[srcIdx][dstIdx] = 0.0f;
        }
    }

    const int32_t rootingBusCount{std::min(m_BusCount, DstBusCount)};
    const int32_t rootingChannelCount{std::min(m_ChannelCount, m_ReceiverChannelCountMax)};
    for (int32_t bus{0}; bus < rootingBusCount; ++bus) {
        for (int32_t channel{0}; channel < rootingChannelCount; ++channel) {
            m_ppSendVolume[channel + bus * m_ChannelCount]
                          [channel + bus * m_ReceiverChannelCountMax] = 1.0f;
        }
    }

    m_SubMixVolume.SetMute(false);
    m_SubMixVolume.SetVolume(1.0f, 0.0f);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    m_pIsSoundSendClampEnabledArray = reinterpret_cast<bool*>(ptr);
    ptr += m_BusCount * sizeof(bool);
#else
    m_pIsSoundSendClampEnabledArray = allocator.Allocate<bool>(m_BusCount);
#endif
    for (int i{0}; i < m_BusCount; ++i) {
        m_pIsSoundSendClampEnabledArray[i] = param.IsSoundSendClampEnabled(i);
    }

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    ptr = util::BytePtr(ptr).AlignUp(alignof(VolumeData)).Get<uint8_t>();
    m_pBusVolume = reinterpret_cast<VolumeData*>(ptr);
    ptr += m_BusCount * sizeof(VolumeData);
#else
    m_pBusVolume = allocator.Allocate<VolumeData>(m_BusCount);
#endif
    for (int i{0}; i < m_BusCount; ++i) {
        new (&m_pBusVolume[i]) VolumeData;
    }

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    m_pChannelVolume = reinterpret_cast<VolumeData*>(ptr);
#else
    m_pChannelVolume = allocator.Allocate<VolumeData>(m_ChannelCount);
#endif
    for (int i{0}; i < m_ChannelCount; ++i) {
        new (&m_pChannelVolume[i]) VolumeData;
    }

    bool isSuccess{false};
    {
        detail::driver::HardwareManager::UpdateAudioRendererScopedLock lock;
        isSuccess = audio::AcquireSubMix(&config, &m_SubMix, sampleRate, mixBufferCount);
    }

    m_IsInitialized = isSuccess;
    if (isSuccess)
        detail::driver::HardwareManager::GetInstance().AddSubMix(this);

    return isSuccess;
}

}  // namespace nn::atk
