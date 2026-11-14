#include <nn/atk/atk_SubMix.h>

#include <nn/util/util_BitUtil.h>

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

bool SubMix::SubMixParam::GetSoundSendClampEnabled(int32_t bus) const {
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

}  // namespace nn::atk
