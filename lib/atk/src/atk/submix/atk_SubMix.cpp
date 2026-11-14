#include <nn/atk/atk_SubMix.h>

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

SubMix::SubMix() = default;

}  // namespace nn::atk
