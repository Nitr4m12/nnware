#include <nn/atk/atk_MultiVoice.h>

#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail::driver {

namespace {

size_t DspAdpcmFrameToNibbleAddress(position_t frame, size_t nibbleAddress) {}

void PanCurveToPanInfo(Util::PanInfo& panInfo, PanCurve curve) {}

void CalcBarancePanForStereo(float* left, float* right, const float& pan, int32_t channelIndex,
                             const Util::PanInfo& panInfo, OutputMode mode) {}

void CalcSurroundPanForSurround(float* front, float* rear, const float& span,
                                const Util::PanInfo& panInfo) {}

void CalcPanForMono(float* left, float* right, const Util::PanInfo& panInfo) {}

void CalcDualPanForStereo(float* left, float* right, const float& pan, const Util::PanInfo& panInfo,
                          OutputMode mode) {}

void CalcSurroundPanForMono(float* front, float* rear, const Util::PanInfo& panInfo) {}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
int32_t GetOutputReceiverMixBufferIndex(const OutputReceiver* pOutputReceiver, int32_t channel,
                                        int32_t bus) {}
#endif

}  // anonymous namespace

const float MultiVoice::VolumeMin{0.0};
const float MultiVoice::VolumeDefault{1.0};
const float MultiVoice::VolumeMax{2.0};

const float MultiVoice::PanLeft{-1.0};
const float MultiVoice::PanCenter{-0.0};
const float MultiVoice::PanRight{1.0};

const float MultiVoice::SpanFront{0.0};
const float MultiVoice::SpanCenter{1.0};
const float MultiVoice::SpanRear{2.0};

const float MultiVoice::CutoffFreqMin{0.0};
const float MultiVoice::CutoffFreqMax{1.0};

const float MultiVoice::BiquadValueMin{0.0};
const float MultiVoice::BiquadValueMax{1.0};

const float MultiVoice::SendMin{0.0};
const float MultiVoice::SendMax{1.0};

MultiVoice::MultiVoice() = default;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
MultiVoice::MultiVoice(OutputAdditionalParam* pAdditionalParam)
    : m_pTvAdditionalParam(pAdditionalParam) {}
#endif

MultiVoice::~MultiVoice() {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].Free();
}

bool MultiVoice::Alloc(int32_t channelCount, int32_t priority, VoiceCallback callback,
                       void* callbackData) {
    channelCount = fnd::Clamp(channelCount, 1, 2);
    m_ChannelCount = 0;

    int allocCount{0};

    for (int i{0}; i < channelCount; ++i) {
        if (!m_Voice[i].AllocVoice(priority))
            break;

        ++allocCount;
    }

    if (allocCount != channelCount) {
        for (int i{0}; i < allocCount; ++i)
            m_Voice[i].Free();

        return false;
    }

    m_ChannelCount = allocCount;
    InitParam(callback, callbackData);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_pTvAdditionalParam != nullptr)
        m_pTvAdditionalParam->Reset();
#endif

    m_IsActive = true;
    return true;
}

void MultiVoice::Free() {
    if (!m_IsActive)
        return;

    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].Free();

    m_ChannelCount = 0;
    MultiVoiceManager::GetInstance().FreeVoice(this);
    m_IsActive = false;
}

void MultiVoice::Start() {
    m_IsStart = true;
    m_IsPause = false;
    m_SyncFlag |= UpdateStart;
}

void MultiVoice::Stop() {
    if (m_IsStarted) {
        StopAllSdkVoice();
        m_IsStarted = false;
    }

    m_IsPausing = false;
    m_IsPause = false;
    m_IsStart = false;
}

void MultiVoice::UpdateVoiceStatus() {
    if (!m_IsActive || !m_IsStarted)
        return;

    if (IsPlayFinished()) {
        if (m_Callback)
            m_Callback(this, VoiceCallbackStatus_FinishWave, m_pCallbackData);

        m_IsStart = false;
        m_IsStarted = false;
    } else {
        bool dropFlag{false};
        for (int ch{0}; ch < m_ChannelCount; ++ch) {
            if (!m_Voice[ch].IsAvailable()) {
                dropFlag = true;
                break;
            }
        }

        if (!dropFlag)
            return;

        for (int ch{0}; ch < m_ChannelCount; ++ch) {
            m_Voice[ch].Free();
        }

        m_ChannelCount = 0;
        m_IsActive = false;

        MultiVoiceManager::GetInstance().FreeVoice(this);

        if (m_Callback)
            m_Callback(this, VoiceCallbackStatus_DropDsp, m_pCallbackData);

        m_IsStart = false;
        m_IsStarted = false;
    }
}

void MultiVoice::Pause(bool flag) {
    if (m_IsPause == flag)
        return;

    m_IsPause = flag;
    m_SyncFlag |= UpdatePause;
}

void MultiVoice::Calc() {
    if (!m_IsStart)
        return;

    if (m_SyncFlag & UpdateSrc) {
        CalcSrc(false);
        m_SyncFlag &= ~UpdateSrc;
    }

    if (m_SyncFlag & UpdateVe) {
        CalcVe();
        m_SyncFlag &= ~UpdateVe;
    }

    if (m_SyncFlag & UpdateMix) {
        CalcMix();
        m_SyncFlag &= ~UpdateMix;
    }

    if (m_SyncFlag & UpdateLpf) {
        CalcLpf();
        m_SyncFlag &= ~UpdateLpf;
    }

    if (m_SyncFlag & UpdateBiquad) {
        CalcBiquadFilter();
        m_SyncFlag &= ~UpdateBiquad;
    }
}

void MultiVoice::Update() {
    if (!m_IsActive)
        return;

    enum {
        NONE,
        RUN,
        PAUSE,
    } runFlag{NONE};

    if ((m_SyncFlag & UpdateStart) && m_IsStart && !m_IsStarted) {
        CalcSrc(true);
        CalcMix();
        CalcVe();
        runFlag = RUN;
        m_IsStarted = true;
        m_SyncFlag &= ~(UpdateStart | UpdateSrc | UpdateMix);
    }

    if (m_IsStarted && (m_SyncFlag & UpdatePause) && m_IsStart) {
        runFlag = m_IsPause ? PAUSE : RUN;
        m_IsPausing = m_IsPause;
        m_SyncFlag &= ~UpdatePause;
    }

    for (int i{0}; i < m_ChannelCount; ++i)
        m_Voice[i].UpdateParam();

    switch (runFlag) {
    case NONE:
        break;
    case RUN:
        RunAllSdkVoice();
        break;
    case PAUSE:
        PauseAllSdkVoice();
        break;
    }
}

void MultiVoice::SetSampleFormat(SampleFormat format) {
    m_Format = format;
    for (int i{0}; i < m_ChannelCount; ++i)
        m_Voice[i].SetSampleFormat(format);
}

void MultiVoice::SetSampleRate(int32_t sampleRate) {
    for (int i{0}; i < m_ChannelCount; ++i)
        m_Voice[i].SetSampleRate(sampleRate);
}

void MultiVoice::SetVolume(float volume) {
    volume = volume < 0 ? 0 : volume;

    if (volume != m_Volume) {
        m_Volume = volume;
        m_SyncFlag |= UpdateVe;
    }
}

void MultiVoice::SetPitch(float pitch) {
    if (m_Pitch != pitch) {
        m_Pitch = pitch;
        m_SyncFlag |= UpdateSrc;
    }
}

void MultiVoice::SetPanMode(PanMode panMode) {
    if (m_PanMode != panMode) {
        m_PanMode = panMode;
        m_SyncFlag |= UpdateMix;
    }
}

void MultiVoice::SetPanCurve(PanCurve panCurve) {
    if (m_PanCurve != panCurve) {
        m_PanCurve = panCurve;
        m_SyncFlag |= UpdateMix;
    }
}

void MultiVoice::SetLpfFreq(float lpfFreq) {
    if (m_LpfFreq != lpfFreq) {
        m_LpfFreq = lpfFreq;
        m_SyncFlag |= UpdateLpf;
    }
}

void MultiVoice::InitParam(VoiceCallback callback, void* callbackData) {
    m_SyncFlag = 0;
    m_pLastWaveBuffer = nullptr;
    m_IsStart = false;
    m_IsStarted = false;
    m_IsPause = false;
    m_IsPausing = false;
    m_Callback = callback;
    m_pCallbackData = callbackData;
    m_Volume = VolumeDefault;
    m_Pitch = VolumeDefault;
    m_PanMode = PanMode_Dual;
    m_PanCurve = PanCurve_Sqrt;
    m_LpfFreq = CutoffFreqMax;
    m_BiquadType = BiquadFilterType_None;
    m_IsEnableFrontBypass = false;
    m_BiquadValue = BiquadValueMin;
    m_TvParam.Initialize();
}

void MultiVoice::CalcSrc([[maybe_unused]] bool initialUpdate) {
    float ratio{m_Pitch};

    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetPitch(ratio);
}

void MultiVoice::CalcVe() {
    float volume{m_Volume * HardwareManager::GetInstance().GetOutputVolume()};

    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetVolume(volume);
}

void MultiVoice::CalcMix() {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex) {
        Voice& voice{m_Voice[channelIndex]};

        PreMixVolume preMix;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        CalcPreMixVolume(&preMix, m_TvParam, channelIndex, OutputDevice_Main);
#else
        CalcPreMixVolume(&preMix, m_TvParam, m_pTvAdditionalParam, channelIndex, OutputDevice_Main);
#endif

        {
            OutputMix tvMix;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
            tvMix.Initialize();
#endif
            CalcTvMix(&tvMix, preMix);
            voice.SetTvMix(tvMix);
        }
    }
}

void MultiVoice::CalcLpf() {
    uint16_t freq{Util::CalcLpfFreq(m_LpfFreq)};

    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex) {
        Voice& voice{m_Voice[channelIndex]};
        // 16000 = Util::CalcLpfFreqTableSize[Util::CalcLpfFreqTableSize - 1]
        if (freq >= 16000)
            voice.SetMonoFilter(false, 0);
        else
            voice.SetMonoFilter(true, freq);
    }
}

void MultiVoice::CalcBiquadFilter() {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex) {
        Voice& voice{m_Voice[channelIndex]};

        const BiquadFilterCallback* cb{
            HardwareManager::GetInstance().GetBiquadFilterCallback(m_BiquadType),
        };

        if (cb != nullptr) {
            if (m_BiquadValue <= 0.0) {
                voice.SetBiquadFilter(false, nullptr);
            } else {
                BiquadFilterCallback::Coefficients coef;
                cb->GetCoefficients(&coef, m_BiquadType, m_BiquadValue);
                voice.SetBiquadFilter(true, &coef);
            }
        } else {
            voice.SetBiquadFilter(false, nullptr);
        }
    }
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void MultiVoice::CalcTvMix(OutputMix* mix, const PreMixVolume& preMix) {
    CalcMixImpl(mix, OutputDeviceIndex_Main, m_TvParam, preMix);
}
#else
void MultiVoice::CalcTvMix(OutputMix* mix, const PreMixVolume& preMix) {
    CalcMixImpl(mix, OutputDeviceIndex_Main, m_TvParam, m_pTvAdditionalParam, preMix);
}
#endif

void MultiVoice::RunAllSdkVoice() {
    for (int32_t channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetState(VoiceState_Play);
}

void MultiVoice::StopAllSdkVoice() {
    for (int32_t channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetState(VoiceState_Stop);
}

void MultiVoice::PauseAllSdkVoice() {
    for (int32_t channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetState(VoiceState_Pause);
}

bool MultiVoice::IsPlayFinished() const {
    if (m_ChannelCount <= 0)
        return false;

    if (m_pLastWaveBuffer == nullptr)
        return false;

    if (m_pLastWaveBuffer->status == WaveBuffer::Status_Wait ||
        m_pLastWaveBuffer->status == WaveBuffer::Status_Play)
        return false;

    return true;
}

}  // namespace nn::atk::detail::driver
