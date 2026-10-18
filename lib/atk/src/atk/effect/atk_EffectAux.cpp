#include <nn/atk/atk_EffectAux.h>

#include <nn/audio.h>
#include <nn/util.h>

#include <nn/atk/atk_HardwareManager.h>

namespace nn::atk {

EffectAux::EffectAux() {
    ResetChannelIndex();
}

void EffectAux::ResetChannelIndex() {
    for (int i{0}; i < ChannelCountMax; ++i)
        m_ChannelSetting[i] = static_cast<ChannelIndex>(i);
}

EffectAux::~EffectAux() = default;

bool EffectAux::Initialize() {
    return true;
}

void EffectAux::Finalize() {}

void EffectAux::OnChangeOutputMode() {}

size_t EffectAux::GetRequiredMemSize(const audio::AudioRendererParameter& parameter) const {
    size_t bufferSize{
        audio::GetRequiredBufferSizeForAuxSendReturnBuffer(&parameter, m_AudioFrameCount,
                                                           m_ChannelCount),
    };
    return bufferSize * 3;
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool EffectAux::AddEffect(audio::AudioRendererConfig* pConfig,
                          const audio::AudioRendererParameter& parameter,
                          audio::FinalMixType* pFinalMix) {
    if (m_IsActive)
        return false;

    BufferSet bufferSet;
    size_t sendReturnBufferSize{
        audio::GetRequiredBufferSizeForAuxSendReturnBuffer(&parameter, m_AudioFrameCount,
                                                           m_ChannelCount),
    };

    SplitEffectBuffer(&bufferSet, m_EffectBuffer, sendReturnBufferSize);
    m_AuxReadBuffer = static_cast<int32_t*>(bufferSet.readBuffer);
    {
        m_AudioRendererUpdateCountWhenAddedAux =
            detail::driver::HardwareManager::GetInstance().GetAudioRendererUpdateCount();
        nn::Result result;
        result = audio::AddAux(pConfig, &m_AuxType, pFinalMix, bufferSet.sendBuffer,
                               bufferSet.returnBuffer, sendReturnBufferSize);

        if (!result.IsSuccess())
            return false;
    }

    audio::SetAuxEnabled(&m_AuxType, m_IsEnabled);
    m_IsActive = true;
    return true;
}

void EffectAux::SplitEffectBuffer(BufferSet* pOutBufferSet, void* effectBuffer,
                                  size_t sendReturnBufferSize) {
    util::BytePtr bytePtr{effectBuffer};
    bytePtr.AlignUp(64);

    pOutBufferSet->sendBuffer = bytePtr.Get();
    bytePtr.Advance(sendReturnBufferSize).AlignUp(64);

    pOutBufferSet->returnBuffer = bytePtr.Get();
    bytePtr.Advance(sendReturnBufferSize).AlignUp(64);

    pOutBufferSet->readBuffer = bytePtr.Get();
}

bool EffectAux::AddEffect(audio::AudioRendererConfig* pConfig,
                          const audio::AudioRendererParameter& parameter,
                          audio::SubMixType* pSubMix) {
    if (m_IsActive)
        return false;

    BufferSet bufferSet;
    size_t sendReturnBufferSize{
        audio::GetRequiredBufferSizeForAuxSendReturnBuffer(&parameter, m_AudioFrameCount,
                                                           m_ChannelCount),
    };

    SplitEffectBuffer(&bufferSet, m_EffectBuffer, sendReturnBufferSize);
    m_AuxReadBuffer = static_cast<int32_t*>(bufferSet.readBuffer);
    {
        m_AudioRendererUpdateCountWhenAddedAux =
            detail::driver::HardwareManager::GetInstance().GetAudioRendererUpdateCount();
        nn::Result result;
        result = audio::AddAux(pConfig, &m_AuxType, pSubMix, bufferSet.sendBuffer,
                               bufferSet.returnBuffer, sendReturnBufferSize);

        if (!result.IsSuccess())
            return false;
    }

    audio::SetAuxEnabled(&m_AuxType, m_IsEnabled);
    m_IsActive = true;
    return true;
}
#else
bool EffectAux::AddEffect(audio::AudioRendererConfig* pConfig,
                          const audio::AudioRendererParameter& parameter,
                          OutputMixer* pOutputMixer) {
    if (m_IsActive)
        return false;

    BufferSet bufferSet;
    size_t sendReturnBufferSize{
        audio::GetRequiredBufferSizeForAuxSendReturnBuffer(&parameter, m_AudioFrameCount,
                                                           m_ChannelCount),
    };

    SplitEffectBuffer(&bufferSet, m_EffectBuffer, sendReturnBufferSize);
    m_AuxReadBuffer = static_cast<int32_t*>(bufferSet.readBuffer);
    {
        m_AudioRendererUpdateCountWhenAddedAux =
            detail::driver::HardwareManager::GetInstance().GetAudioRendererUpdateCount();
        nn::Result result;
        switch (pOutputMixer->GetReceiverType()) {
        case OutputReceiver::ReceiverType::ReceiverType_SubMix:
            result = audio::AddAux(
                pConfig, &m_AuxType, static_cast<SubMix*>(pOutputMixer)->GetAudioSubMixInstance(),
                bufferSet.sendBuffer, bufferSet.returnBuffer, sendReturnBufferSize);
            break;
        case OutputReceiver::ReceiverType::ReceiverType_FinalMix:
            result =
                audio::AddAux(pConfig, &m_AuxType,
                              static_cast<FinalMix*>(pOutputMixer)->GetAudioFinalMixInstance(),
                              bufferSet.sendBuffer, bufferSet.returnBuffer, sendReturnBufferSize);
            break;
        default:
            NN_UNEXPECTED_DEFAULT;
        }

        if (!result.IsSuccess())
            return false;
    }

    audio::SetAuxEnabled(&m_AuxType, m_IsEnabled);
    m_IsActive = true;
    return true;
}

void EffectAux::SplitEffectBuffer(BufferSet* pOutBufferSet, void* effectBuffer,
                                  size_t sendReturnBufferSize) {
    util::BytePtr bytePtr{effectBuffer};
    bytePtr.AlignUp(64);

    pOutBufferSet->sendBuffer = bytePtr.Get();
    bytePtr.Advance(sendReturnBufferSize).AlignUp(64);

    pOutBufferSet->returnBuffer = bytePtr.Get();
    bytePtr.Advance(sendReturnBufferSize).AlignUp(64);

    pOutBufferSet->readBuffer = bytePtr.Get();
}
#endif

}  // namespace nn::atk
