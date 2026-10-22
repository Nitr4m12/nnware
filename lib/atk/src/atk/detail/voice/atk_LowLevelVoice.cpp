#include <nn/atk/atk_LowLevelVoice.h>

#include <nn/audio.h>

#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace {

const int32_t LowLevelVoiceInitialPriority{0};
const int LowLevelVoiceInitialSampleRate{32000};
const nn::atk::SampleFormat LowLevelVoiceInitialSampleFormat{nn::atk::SampleFormat_PcmS16};
const nn::atk::detail::VoiceState LowLevelVoiceInitialVoiceState{nn::atk::detail::VoiceState_Stop};

const size_t LowLevelVoiceAlignedSize{sizeof(nn::atk::detail::LowLevelVoice)};

int ConvertAtkPriorityToAudioPriority(int32_t atkPriority) {
    if (atkPriority == 255)
        return 0;

    return 255 - atkPriority;
}

int GetOutputReceiverMixBufferIndex(const nn::atk::OutputReceiver* pOutputReceiver, int channel,
                                    int bus) {
    return channel + bus * pOutputReceiver->GetChannelCount();
}

}  // anonymous namespace

namespace nn::atk::detail {

LowLevelVoice::LowLevelVoice()
    : m_AdpcmParam(), m_Voice(), m_VoiceParam(), m_Priority{LowLevelVoiceInitialPriority},
      m_State{LowLevelVoiceInitialVoiceState}, m_SampleRate{LowLevelVoiceInitialSampleRate},
      m_SampleFormat{LowLevelVoiceInitialSampleFormat} {}

void LowLevelVoice::Initialize() {
    m_Priority = LowLevelVoiceInitialPriority;
    m_IsAvailable = true;
    m_SampleRate = LowLevelVoiceInitialSampleRate;
    m_SampleFormat = LowLevelVoiceInitialSampleFormat;
    m_PlayPosition = 0;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = 0;
#else
    m_pOutputReceiver = nullptr;
#endif
    m_NodeId = 0;
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    m_IsRun = true;
#endif
}

void LowLevelVoice::Finalize() {
    FreeAllWaveBuffer();

    if (m_IsSetVoiceSlot) {
        audio::ReleaseVoiceSlot(&driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
                                &m_Voice);
        m_IsSetVoiceSlot = false;
    }

    m_pVoice = nullptr;
    m_IsAvailable = false;
    m_State = VoiceState_Stop;
}

bool LowLevelVoice::IsAvailable() const {
    return m_IsAvailable;
}

void LowLevelVoice::SetAvailable(bool isAvailable) {
    m_IsAvailable = isAvailable;
}

bool LowLevelVoice::IsVoiceDroppedFlagOn() const {
    if (audio::IsVoiceValid(&m_Voice))
        return audio::IsVoiceDroppedFlagOn(&m_Voice);

    return false;
}

void LowLevelVoice::AppendWaveBuffer(WaveBuffer* waveBuffer) {
    waveBuffer->next = nullptr;
    waveBuffer->status = WaveBuffer::Status_Wait;
    if (m_WaveBufferListEnd == nullptr)
        m_WaveBufferListBegin = waveBuffer;
    else
        m_WaveBufferListEnd->next = waveBuffer;

    m_WaveBufferListEnd = waveBuffer;
}

void LowLevelVoice::FreeAllWaveBuffer() {
    for (WaveBuffer* waveBuffer{m_WaveBufferListBegin}; waveBuffer != nullptr;
         waveBuffer = waveBuffer->next)
        waveBuffer->status = WaveBuffer::Status_Done;

    m_WaveBufferListBegin = nullptr;
    m_WaveBufferListEnd = nullptr;
    m_LastAppendBuffer = nullptr;
}

void LowLevelVoice::UpdateState(OutputMode outputMode) {
    if (!audio::IsVoiceValid(&m_Voice))
        return;

    bool isRun = audio::GetVoicePlayState(&m_Voice) == audio::VoiceType::PlayState_Play;

    switch (m_State) {
    case VoiceState_Play:
        UpdateStatePlay(isRun, outputMode);
        break;

    case VoiceState_Stop:
        UpdateStateStop(isRun);
        break;

    case VoiceState_Pause:
        UpdateStatePause(isRun, outputMode);
        break;
    }
}

void LowLevelVoice::SetPriority(int32_t priority) {
    m_Priority = priority;

    if (!audio::IsVoiceValid(&m_Voice))
        return;

    audio::SetVoicePriority(&m_Voice, ConvertAtkPriorityToAudioPriority(m_Priority));
}

void LowLevelVoice::SetState(VoiceState state) {
    m_State = state;

    switch (state) {
    case VoiceState_Play:
        if (!m_IsSetVoiceSlot) {
            AllocVoice();
            m_IsSetVoiceSlot = true;
        }
        break;

    case VoiceState_Stop:
        if (m_IsSetVoiceSlot) {
            audio::ReleaseVoiceSlot(
                &driver::HardwareManager::GetInstance().GetAudioRendererConfig(), &m_Voice);
            m_IsSetVoiceSlot = false;
        }
        break;

    case VoiceState_Pause:
        break;
    }
}

void LowLevelVoice::UpdateVoiceInfo(VoiceInfo* voiceInfo) const {
    voiceInfo->userId = m_pVoice;
    voiceInfo->voiceState = m_State;

    volatile WaveBuffer* pListBegin{m_WaveBufferListBegin};

    if (m_WaveBufferListBegin != nullptr) {
        voiceInfo->waveBufferStatus = pListBegin->status;
        voiceInfo->waveBufferTag = pListBegin->userParam;
    } else {
        voiceInfo->waveBufferStatus = WaveBuffer::Status_Free;
    }

    voiceInfo->playPosition = m_PlayPosition;
}

void LowLevelVoice::UpdateVoiceParam(const VoiceParam& voiceParam, OutputMode outputMode) {
    UpdateVolume(voiceParam);
    UpdatePitch(voiceParam);
    UpdateBiquadFilter(voiceParam);
    UpdateLowPassFilter(voiceParam);
    UpdateMixVolume(voiceParam.m_TvMix, outputMode);
}

bool LowLevelVoice::AllocVoice() {
    const audio::AdpcmParameter* pParameter{nullptr};
    int32_t paramSize{0};

    audio::SampleFormat sampleFormat{audio::SampleFormat_PcmInt16};
    switch (m_SampleFormat) {
    case SampleFormat_PcmS8:
        sampleFormat = audio::SampleFormat_PcmInt8;
        break;
    case SampleFormat_DspAdpcm:
        sampleFormat = audio::SampleFormat_Adpcm;
        pParameter = &m_AdpcmParam;
        paramSize = sizeof(audio::AdpcmParameter);
        break;
    default:
        break;
    }

    int32_t audioPriority{ConvertAtkPriorityToAudioPriority(m_Priority)};

    bool result{
        audio::AcquireVoiceSlot(&driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
                                &m_Voice, m_SampleRate, 1, sampleFormat, audioPriority, pParameter,
                                paramSize),
    };

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    if (driver::HardwareManager::GetInstance().IsPresetSubMixEnabled()) {
        audio::SetVoiceDestination(
            &driver::HardwareManager::GetInstance().GetAudioRendererConfig(), &m_Voice,
            &driver::HardwareManager::GetInstance().GetSubMix(m_SubMixIndex));
    } else {
        audio::SetVoiceDestination(&driver::HardwareManager::GetInstance().GetAudioRendererConfig(),
                                   &m_Voice, &driver::HardwareManager::GetInstance().GetFinalMix());
    }
#else
    switch (m_pOutputReceiver->GetReceiverType()) {
    case OutputReceiver::ReceiverType::ReceiverType_SubMix:
        audio::SetVoiceDestination(
            &driver::HardwareManager::GetInstance().GetAudioRendererConfig(), &m_Voice,
            static_cast<SubMix*>(m_pOutputReceiver)->GetAudioSubMixInstance());
        break;
    case OutputReceiver::ReceiverType::ReceiverType_FinalMix:
        audio::SetVoiceDestination(
            &driver::HardwareManager::GetInstance().GetAudioRendererConfig(), &m_Voice,
            static_cast<FinalMix*>(m_pOutputReceiver)->GetAudioFinalMixInstance());
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
#endif

    m_NodeId = audio::GetVoiceNodeId(&m_Voice);
    return result;
}

void LowLevelVoice::UpdateStatePlay(bool isRun, OutputMode outputMode) {
    UpdateWaveBuffer(isRun, outputMode);
    UpdateVoiceParam(m_VoiceParam, outputMode);
}

void LowLevelVoice::UpdateStateStop(bool isRun) {
    if (!isRun)
        return;

    audio::SetVoicePlayState(&m_Voice, audio::VoiceType::PlayState_Stop);
}

void LowLevelVoice::UpdateStatePause(bool isRun, OutputMode outputMode) {
    if (!isRun)
        return;

    UpdateVoiceParam(m_VoiceParam, outputMode);
    audio::SetVoicePlayState(&m_Voice, audio::VoiceType::PlayState_Pause);
}

void LowLevelVoice::UpdatePlayPosition() {
    if (m_IsSetVoiceSlot) {
        int64_t playedSampleCount{audio::GetVoicePlayedSampleCount(&m_Voice)};
        int64_t startOffset{0};

        WaveBuffer* currentWaveBuffer{m_WaveBufferListBegin};

        if (currentWaveBuffer != nullptr) {
            startOffset = currentWaveBuffer->sampleOffset;
            int64_t loopSampleLength{currentWaveBuffer->sampleLength - startOffset};

            if (currentWaveBuffer->loopFlag && playedSampleCount > loopSampleLength)
                playedSampleCount =
                    playedSampleCount - (playedSampleCount / loopSampleLength) * loopSampleLength;
        }

        m_PlayPosition = playedSampleCount + startOffset;
    }
}

void LowLevelVoice::UpdateWaveBuffer(bool isRun, OutputMode outputMode) {
    if (isRun) {
        UpdateWaveBufferOnPlayState();
        UpdatePlayPosition();
    } else {
        UpdateWaveBufferOnStopState(outputMode);
    }
}

void LowLevelVoice::UpdateWaveBufferOnPlayState() {
    WaveBuffer* currentWaveBuffer{m_WaveBufferListBegin};
    if (currentWaveBuffer == nullptr)
        return;

    const audio::WaveBuffer* pWaveBuffer{audio::GetReleasedWaveBuffer(&m_Voice)};
    if (pWaveBuffer != nullptr) {
        m_WaveBufferListBegin = currentWaveBuffer->next;
        currentWaveBuffer->status = WaveBuffer::Status_Done;
        if (m_WaveBufferListBegin == nullptr) {
            m_WaveBufferListEnd = nullptr;
            audio::SetVoicePlayState(&m_Voice, audio::VoiceType::PlayState_Stop);
        } else {
            m_WaveBufferListBegin->status = WaveBuffer::Status_Play;
        }
    }

    if (m_LastAppendBuffer->next == nullptr)
        return;

    if (AppendWaveBufferToVoice(m_LastAppendBuffer->next)) {
        m_LastAppendBuffer = m_LastAppendBuffer->next;
    }
}

void LowLevelVoice::UpdateWaveBufferOnStopState(OutputMode outputMode) {
    WaveBuffer* currentWaveBuffer{m_WaveBufferListBegin};
    if (currentWaveBuffer == nullptr)
        return;

    if (audio::GetVoicePlayState(&m_Voice) == audio::VoiceType::PlayState_Pause) {
        audio::SetVoicePlayState(&m_Voice, audio::VoiceType::PlayState_Play);
        return;
    }

    switch (currentWaveBuffer->status) {
    case WaveBuffer::Status_Play:
        FreeAllWaveBuffer();
        break;
    case WaveBuffer::Status_Wait:
        AppendWaveBufferToVoice(currentWaveBuffer);
        m_LastAppendBuffer = currentWaveBuffer;
        audio::SetVoicePlayState(&m_Voice, audio::VoiceType::PlayState_Play);
        UpdateMixVolume(m_VoiceParam.m_TvMix, outputMode);
        currentWaveBuffer->status = WaveBuffer::Status_Play;
        break;
    case WaveBuffer::Status_Done:
    case WaveBuffer::Status_Free:
        break;
    }
}

bool LowLevelVoice::AppendWaveBufferToVoice(WaveBuffer* waveBuffer) {
    const audio::AdpcmContext* pContext{nullptr};
    size_t contextSize{0};
    if (m_SampleFormat == SampleFormat_DspAdpcm) {
        pContext = &waveBuffer->pAdpcmContext->audioAdpcmContext;
        contextSize = 6;
    }

    audio::WaveBuffer audioWaveBuffer;
    audioWaveBuffer.buffer = waveBuffer->bufferAddress;
    audioWaveBuffer.size = waveBuffer->bufferSize;
    audioWaveBuffer.startSampleOffset = waveBuffer->sampleOffset;
    audioWaveBuffer.endSampleOffset = waveBuffer->sampleLength;
    audioWaveBuffer.loop = waveBuffer->loopFlag;
    audioWaveBuffer.pContext = pContext;
    audioWaveBuffer.contextSize = contextSize;
    audioWaveBuffer.isEndOfStream = true;

    return audio::AppendWaveBuffer(&m_Voice, &audioWaveBuffer);
}

void LowLevelVoice::UpdateMixVolume(const OutputMix& outputMix, OutputMode outputMode) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    if (driver::HardwareManager::GetInstance().IsPresetSubMixEnabled()) {
        UpdateMixVolumeOnSubMix(outputMix, outputMode);
    } else {
        UpdateMixVolumeOnFinalMix(outputMix, outputMode);
    }
#else
    const bool isSurroundMode{outputMode == OutputMode_Surround};
    const bool isEffectEnabled{driver::HardwareManager::GetInstance().IsEffectInitialized()};
    const int busCount{isEffectEnabled ? m_pOutputReceiver->GetBusCount() : 1};
    const int channelCount{m_pOutputReceiver->GetChannelCount()};

    for (int busIndex{0}; busIndex < busCount; ++busIndex) {
        for (int channelIndex{0}; channelIndex < channelCount; ++channelIndex) {
            const int mixBufferIndex{
                GetOutputReceiverMixBufferIndex(m_pOutputReceiver, channelIndex, busIndex),
            };
            const float volume{
                isSurroundMode   ? GetClampedVoiceVolume(outputMix.channelGain[mixBufferIndex]) :
                channelIndex < 2 ? GetClampedVoiceVolume(outputMix.channelGain[mixBufferIndex]) :
                                   0.0f,
            };
            SetVoiceMixVolume(volume, mixBufferIndex);
        }
    }
#endif
}

void LowLevelVoice::UpdateVolume(const VoiceParam& voiceParam) {
    float volume{GetClampedVoiceVolume(voiceParam.m_Volume)};
    audio::SetVoiceVolume(&m_Voice, volume);
}

void LowLevelVoice::UpdatePitch(const VoiceParam& voiceParam) {
    float pitch{fnd::Clamp(voiceParam.m_Pitch, 0.0625f, 16.0f)};
    audio::SetVoicePitch(&m_Voice, pitch);
}

void LowLevelVoice::UpdateBiquadFilter(const VoiceParam& voiceParam) {
    audio::BiquadFilterParameter parameter;
    parameter.denominator[0] = voiceParam.m_BiquadFilterCoefficients.a1;
    parameter.denominator[1] = voiceParam.m_BiquadFilterCoefficients.a2;
    parameter.numerator[0] = voiceParam.m_BiquadFilterCoefficients.b0;
    parameter.numerator[1] = voiceParam.m_BiquadFilterCoefficients.b1;
    parameter.numerator[2] = voiceParam.m_BiquadFilterCoefficients.b2;
    parameter.enable = voiceParam.m_BiquadFilterFlag;
    audio::SetVoiceBiquadFilterParameter(&m_Voice, 0, parameter);
}

void LowLevelVoice::SetVoiceMixVolume(float mixVolume, int destinationIndex) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    if (driver::HardwareManager::GetInstance().IsPresetSubMixEnabled()) {
        audio::SetVoiceMixVolume(&m_Voice,
                                 &driver::HardwareManager::GetInstance().GetSubMix(m_SubMixIndex),
                                 mixVolume, 0, destinationIndex);
    } else {
        audio::SetVoiceMixVolume(&m_Voice, &driver::HardwareManager::GetInstance().GetFinalMix(),
                                 mixVolume, 0, destinationIndex);
    }
#else
    switch (m_pOutputReceiver->GetReceiverType()) {
    case OutputReceiver::ReceiverType::ReceiverType_SubMix:
        audio::SetVoiceMixVolume(&m_Voice,
                                 static_cast<SubMix*>(m_pOutputReceiver)->GetAudioSubMixInstance(),
                                 mixVolume, 0, destinationIndex);
        break;
    case OutputReceiver::ReceiverType::ReceiverType_FinalMix:
        audio::SetVoiceMixVolume(
            &m_Voice, static_cast<FinalMix*>(m_pOutputReceiver)->GetAudioFinalMixInstance(),
            mixVolume, 0, destinationIndex);
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
#endif
}

float LowLevelVoice::GetClampedVoiceVolume(float volume) {
    return fnd::FloatClamp(volume, 0.0f, 128.0f);
}

}  // namespace nn::atk::detail
