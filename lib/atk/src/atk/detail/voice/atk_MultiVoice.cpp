#include <nn/atk/atk_MultiVoice.h>

#include <algorithm>

#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail::driver {

namespace {

size_t DspAdpcmFrameToNibbleAddress(position_t frame, size_t nibbleAddress) {}

void PanCurveToPanInfo(Util::PanInfo& panInfo, PanCurve curve) {
    switch (curve) {
    case PanCurve_Sqrt:
        panInfo.curve = Util::PanCurve_Sqrt;
        break;
    case PanCurve_Sqrt0Db:
        panInfo.curve = Util::PanCurve_Sqrt;
        panInfo.centerZeroFlag = true;
        break;
    case PanCurve_Sqrt0DbClamp:
        panInfo.curve = Util::PanCurve_Sqrt;
        panInfo.centerZeroFlag = true;
        panInfo.zeroClampFlag = true;
        break;
    case PanCurve_Sincos:
        panInfo.curve = Util::PanCurve_Sincos;
        break;
    case PanCurve_Sincos0Db:
        panInfo.curve = Util::PanCurve_Sincos;
        panInfo.centerZeroFlag = true;
        break;
    case PanCurve_Sincos0DbClamp:
        panInfo.curve = Util::PanCurve_Sincos;
        panInfo.centerZeroFlag = true;
        panInfo.zeroClampFlag = true;
        break;
    case PanCurve_Linear:
        panInfo.curve = Util::PanCurve_Linear;
        break;
    case PanCurve_Linear0Db:
        panInfo.curve = Util::PanCurve_Linear;
        panInfo.centerZeroFlag = true;
        break;
    case PanCurve_Linear0DbClamp:
        panInfo.curve = Util::PanCurve_Linear;
        panInfo.centerZeroFlag = true;
        panInfo.zeroClampFlag = true;
        break;
    default:
        panInfo.curve = Util::PanCurve_Sqrt;
        break;
    }
}

void CalcBarancePanForStereo(float* left, float* right, const float& pan, int32_t channelIndex,
                             const Util::PanInfo& panInfo, OutputMode mode) {
    if (channelIndex == ChannelIndex_FrontLeft) {
        *left = Util::CalcPanRatio(pan, panInfo, mode);
        *right = MultiVoice::PanCenter;
    } else if (channelIndex == ChannelIndex_FrontRight) {
        *left = MultiVoice::PanCenter;
        *right = Util::CalcPanRatio(MultiVoice::PanCenter - pan, panInfo, mode);
    }
}

void CalcSurroundPanForSurround(float* front, float* rear, const float& span,
                                const Util::PanInfo& panInfo) {
    *front = Util::CalcSurroundPanRatio(span, panInfo);
    *rear = Util::CalcSurroundPanRatio(MultiVoice::SpanRear - span, panInfo);
}

void CalcPanForMono(float* left, float* right, const Util::PanInfo& panInfo) {
    *left = Util::CalcPanRatio(MultiVoice::PanCenter, panInfo, OutputMode_Monaural);
    *right = *left;
}

void CalcDualPanForStereo(float* left, float* right, const float& pan, const Util::PanInfo& panInfo,
                          OutputMode mode) {
    *left = Util::CalcPanRatio(pan, panInfo, mode);
    *right = Util::CalcPanRatio(MultiVoice::PanCenter - pan, panInfo, mode);
}

void CalcSurroundPanForMono(float* front, float* rear, const Util::PanInfo& panInfo) {
    *front = Util::CalcSurroundPanRatio(MultiVoice::SpanFront, panInfo);
    *rear = Util::CalcSurroundPanRatio(MultiVoice::SpanRear, panInfo);
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
int32_t GetOutputReceiverMixBufferIndex(const OutputReceiver* pOutputReceiver, int32_t channel,
                                        int32_t bus) {
    return channel + bus * pOutputReceiver->GetChannelCount();
}
#endif

}  // anonymous namespace

const float MultiVoice::VolumeMin{0.0};
const float MultiVoice::VolumeDefault{1.0};
const float MultiVoice::VolumeMax{2.0};

const float MultiVoice::PanLeft{-1.0};
const float MultiVoice::PanCenter{0.0};
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

void MultiVoice::SetBiquadFilter(int32_t type, float value) {
    value = fnd::Clamp(value, BiquadValueMin, BiquadValueMax);

    bool isUpdate{false};

    if (m_BiquadType != type) {
        m_BiquadType = type;
        isUpdate = true;
    }

    if (value != m_BiquadValue) {
        m_BiquadValue = value;
        isUpdate = true;
    }

    if (isUpdate)
        m_SyncFlag |= UpdateBiquad;
}

void MultiVoice::SetPriority(int32_t priority) {
    m_Priority = priority;
    MultiVoiceManager::GetInstance().ChangeVoicePriority(this);

    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetPriority(m_Priority);
}

void MultiVoice::SetOutputLine(uint32_t lineFlag) {
    if (m_OutputLineFlag != lineFlag) {
        m_OutputLineFlag = lineFlag;
        m_SyncFlag |= UpdateMix;
    }
}

void MultiVoice::SetOutputParamImpl(const OutputParam& in, OutputParam& out) {
    {
        float volume = in.volume < VolumeMin ? VolumeMin : in.volume;
        if (volume != out.volume) {
            out.volume = volume;
            m_SyncFlag |= UpdateMix;
        }
    }

    if (in.mixMode != out.mixMode) {
        out.mixMode = in.mixMode;
        m_SyncFlag |= UpdateMix;
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (out.mixMode == MixMode_Mixparameter) {
#endif
        for (int i{0}; i < WaveChannelMax; ++i) {
            for (int j{0}; j < ChannelIndex_Count; ++j) {
                if (in.mixParameter[i].ch[j] != out.mixParameter[i].ch[j]) {
                    out.mixParameter[i].ch[j] = in.mixParameter[i].ch[j];
                    m_SyncFlag |= UpdateMix;
                }
            }
        }
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    }
#endif

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (out.mixMode == MixMode_Pan) {
#endif
        if (in.pan != out.pan) {
            out.pan = in.pan;
            m_SyncFlag |= UpdateMix;
        }

        if (in.span != out.span) {
            out.span = in.span;
            m_SyncFlag |= UpdateMix;
        }
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    }
#endif

    for (int i{0}; i < DefaultBusCount; ++i) {
        float send{in.send[i]};
        if (send != out.send[i]) {
            out.send[i] = send;
            m_SyncFlag |= UpdateMix;
        }
    }
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
// NON_MATCHING: requires MultiVoice::SetOutputVolumeThroughModePacketImpl and missing m_SyncFlag
// set
void MultiVoice::SetOutputAdditionalParamImpl(
    const SendArray* const pAdditionalSend, const BusMixVolumePacket* const pBusMixVolumePacket,
    const OutputBusMixVolume* const pBusMixVolume,
    const VolumeThroughModePacket* const pVolumeThroughModePacket) {
    if (m_pTvAdditionalParam == nullptr)
        return;

    if (pAdditionalSend != nullptr && m_pTvAdditionalParam->IsAdditionalSendEnabled()) {
        const int BusCount{m_pOutputReceiver->GetBusCount()};
        if (BusCount > DefaultBusCount) {
            for (int i{DefaultBusCount}; i < BusCount; ++i) {
                float send{pAdditionalSend->TryGetValue(i - 4)};
                if (send != m_pTvAdditionalParam->TryGetAdditionalSend(i)) {
                    m_pTvAdditionalParam->TrySetAdditionalSend(i, send);
                    m_SyncFlag |= UpdateMix;
                }
            }
        }
    }

    if (pBusMixVolume != nullptr && pBusMixVolumePacket != nullptr &&
        m_pTvAdditionalParam->IsBusMixVolumeEnabled()) {
        SetOutputBusMixVolumeImpl(*pBusMixVolumePacket, *pBusMixVolume,
                                  *m_pTvAdditionalParam->GetBusMixVolumePacketAddr());
    }

    if (pVolumeThroughModePacket != nullptr && m_pTvAdditionalParam->IsVolumeThroughModeEnabled()) {
        SetOutputVolumeThroughModePacketImpl(
            *pVolumeThroughModePacket, *m_pTvAdditionalParam->GetVolumeThroughModePacketAddr());
    }
}

void MultiVoice::SetOutputBusMixVolumeImpl(const BusMixVolumePacket& in,
                                           const OutputBusMixVolume& busMixVolume,
                                           BusMixVolumePacket& out) {
    if (in.IsUsed() != out.IsUsed()) {
        out.SetUsed(in.IsUsed());
        m_SyncFlag |= UpdateMix;
    }

    if (out.IsUsed()) {
        const int BusCount{std::min(in.GetBusCount(), out.GetBusCount())};
        for (int i{0}; i < BusCount; ++i) {
            if (in.IsEnabled(i) != out.IsEnabled(i)) {
                out.SetEnabled(i, in.IsEnabled(i));
                m_SyncFlag |= UpdateMix;
            }
        }

        const int MixBufferCount{m_pOutputReceiver->GetBusCount() *
                                 m_pOutputReceiver->GetChannelCount()};
        for (int i{0}; i < WaveChannelMax; ++i) {
            for (int j{0}; j < MixBufferCount; ++j) {
                if (busMixVolume.volume[i][j] != out.GetBusMixVolume(i, j)) {
                    out.SetBusMixVolume(i, j, busMixVolume.volume[i][j]);
                    m_SyncFlag |= UpdateMix;
                }
            }
        }
    }
}

// WIP
void MultiVoice::SetOutputVolumeThroughModePacketImpl(const VolumeThroughModePacket& in,
                                                      VolumeThroughModePacket& out) {
    bool isChanged{false};
    bool volumeThroughModeUsed{in.IsVolumeThroughModeUsed()};
    if (volumeThroughModeUsed != out.IsVolumeThroughModeUsed()) {
        out.SetVolumeThroughModeUsed(volumeThroughModeUsed);
        if (!out.IsVolumeThroughModeUsed()) {
            m_SyncFlag |= (UpdateVe | UpdateMix);
            return;
        }
        isChanged = true;
    } else {
        m_SyncFlag |= (UpdateVe | UpdateMix);
        return;
    }

    float volume{in.GetBinaryVolume()};
    volume = volume < 0 ? 0 : volume;
    if (volume != out.GetBinaryVolume()) {
        out.SetBinaryVolume(volume);
        m_SyncFlag |= (UpdateVe | UpdateMix);
    }

    const int BusCount{m_pOutputReceiver->GetBusCount()};
    for (int i{0}; i < BusCount; ++i) {
        uint8_t volumeThroughMode{in.TryGetVolumeThroughMode(i)};
        if (volumeThroughMode != out.TryGetVolumeThroughMode(i)) {
            out.TrySetVolumeThroughMode(i, volumeThroughMode);
            m_SyncFlag |= UpdateMix;
        }
    }
}
#endif

void MultiVoice::SetTvParam(const OutputParam& param) {
    SetOutputParamImpl(param, m_TvParam);
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void MultiVoice::SetSubMixIndex(int32_t subMixIndex) {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].SetSubMixIndex(subMixIndex);
}
#endif

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void MultiVoice::SetTvAdditionalParam(const OutputAdditionalParam& param) {
    if (param.IsBusMixVolumeEnabled()) {
        SetOutputAdditionalParamImpl(param.GetAdditionalSendAddr(),
                                     param.GetBusMixVolumePacketAddr(), &param.GetBusMixVolume(),
                                     param.GetVolumeThroughModePacketAddr());
    } else {
        SetOutputAdditionalParamImpl(param.GetAdditionalSendAddr(),
                                     param.GetBusMixVolumePacketAddr(), nullptr,
                                     param.GetVolumeThroughModePacketAddr());
    }
}

void MultiVoice::SetTvAdditionalParam(const SendArray* pAdditionalSend,
                                      const BusMixVolumePacket* pBusMixVolumePacket,
                                      const OutputBusMixVolume* pBusMixVolume,
                                      const VolumeThroughModePacket* pVolumeThroughModePacket) {
    SetOutputAdditionalParamImpl(pAdditionalSend, pBusMixVolumePacket, pBusMixVolume,
                                 pVolumeThroughModePacket);
}

void MultiVoice::SetOutputReceiver(OutputReceiver* pOutputReceiver) {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex) {
        m_Voice[channelIndex].SetOutputReceiver(pOutputReceiver);
        m_pOutputReceiver = pOutputReceiver;
    }
}
#endif

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
void MultiVoice::CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param, int32_t channelIndex,
                                  OutputDevice device)
#else
void MultiVoice::CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param,
                                  const OutputAdditionalParam* const pAdditionalParam,
                                  int32_t channelIndex, OutputDevice device)
#endif
{
    const OutputMode nwMode{HardwareManager::GetInstance().GetOutputMode(device)};

    float fL = 0;
    float fR = 0;
    float rL = 0;
    float rR = 0;
    float fC = 0;
    float lfe = 0;
    if (param.mixMode == MixMode_Pan) {
        float left = 0;
        float right = 0;
        float front = 0;
        float rear = 0;

        Util::PanInfo panInfo;
        PanCurveToPanInfo(panInfo, m_PanCurve);

        switch (nwMode) {
        case OutputMode_Monaural:
            CalcPanForMono(&left, &right, panInfo);
            break;
        case OutputMode_Stereo:
        case OutputMode_Surround: {
            if (m_ChannelCount >= static_cast<int32_t>(WaveChannelMax)) {
                if (m_PanMode == PanMode_Balance) {
                    CalcBarancePanForStereo(&left, &right, param.pan, channelIndex, panInfo,
                                            nwMode);
                    break;
                }
            }

            float voicePan{param.pan};
            if (m_ChannelCount == static_cast<int32_t>(WaveChannelMax)) {
                if (channelIndex == ChannelIndex_FrontLeft)
                    voicePan += PanLeft;
                else if (channelIndex == ChannelIndex_FrontRight)
                    voicePan += PanRight;
            }

            CalcDualPanForStereo(&left, &right, voicePan, panInfo, nwMode);
            break;
        }
        default:
            break;
        }

        switch (nwMode) {
        case OutputMode_Monaural:
        case OutputMode_Stereo:
            CalcSurroundPanForMono(&front, &rear, panInfo);
            break;
        case OutputMode_Surround:
            CalcSurroundPanForSurround(&front, &rear, param.span, panInfo);
            break;
        default:
            break;
        }

        fL = front * left;
        fR = front * right;
        rL = rear * left;
        rR = rear * right;
        lfe = fC;
    } else {
        fL = param.mixParameter[channelIndex].fL;
        fR = param.mixParameter[channelIndex].fR;
        rL = param.mixParameter[channelIndex].rL;
        rR = param.mixParameter[channelIndex].rR;
        fC = param.mixParameter[channelIndex].fC;
        lfe = param.mixParameter[channelIndex].lfe;
    }

    const OutputMode endUserMode{HardwareManager::GetInstance().GetEndUserOutputMode(device)};
    switch (endUserMode) {
    case OutputMode_Monaural: {
        switch (nwMode) {
        case OutputMode_Stereo: {
            float mono{fL + fR};
            mono /= WaveChannelMax;
            fL = mono;
            fR = mono;
            break;
        }
        case OutputMode_Surround: {
            float mono{rL + fL};
            mono += rR + fR;
            mono /= WaveChannelMax;
            fL = mono;
            fR = mono;
            rL = 0.0f;
            rR = 0.0f;
            break;
        }
        default:
            break;
        }
        break;
    }
    case OutputMode_Stereo: {
        switch (nwMode) {
        case OutputMode_Surround: {
            float left{fL + rL};
            float right{fR + rR};

            fR = right;
            fL = left;
            rL = 0.0f;
            rR = 0.0f;
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    mix->volume[ChannelIndex_FrontLeft] = fL;
    mix->volume[ChannelIndex_FrontRight] = fR;
    mix->volume[ChannelIndex_RearLeft] = rL;
    mix->volume[ChannelIndex_RearRight] = rR;
    mix->volume[ChannelIndex_FrontCenter] = fC;
    mix->volume[ChannelIndex_Lfe] = lfe;
#else
    for (int i{0}; i < OutputMix::ChannelCountMax; ++i) {
        const int BusIndex{i / m_pOutputReceiver->GetChannelCount()};
        if (BusIndex >= m_pOutputReceiver->GetBusCount()) {
            mix->volume[i] = 0;
            continue;
        }

        if (pAdditionalParam != nullptr && pAdditionalParam->IsBusMixVolumeEnabled() &&
            pAdditionalParam->IsBusMixVolumeUsed() &&
            pAdditionalParam->IsBusMixVolumeEnabledForBus(BusIndex)) {
            mix->volume[i] = pAdditionalParam->GetBusMixVolume(channelIndex, i);
            continue;
        }

        const int ChannelIndex{i % m_pOutputReceiver->GetChannelCount()};
        switch (ChannelIndex) {
        case ChannelIndex_FrontLeft:
            mix->volume[i] = fL;
            break;
        case ChannelIndex_FrontRight:
            mix->volume[i] = fR;
            break;
        case ChannelIndex_RearLeft:
            mix->volume[i] = rL;
            break;
        case ChannelIndex_RearRight:
            mix->volume[i] = rR;
            break;
        case ChannelIndex_FrontCenter:
            mix->volume[i] = fC;
            break;
        case ChannelIndex_Lfe:
            mix->volume[i] = lfe;
            break;
        default:
            mix->volume[i] = 0;
            break;
        }
    }
#endif
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

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
// NON_MATCHING: mix->channelGain gets trapped in a loop, when all values
// should be assigned independently
void MultiVoice::CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                             const PreMixVolume& pre) {
    float mainSend{0.0f};
    float fxSend[AuxBus_Count];
    for (int i{0}; i < AuxBus_Count; ++i)
        fxSend[i] = 0;

    uint32_t outputDeviceFlag{0};
    for (int i{0}; i < OutputLineIndex_Max; ++i) {
        if (m_OutputLineFlag & (1 << i))
            outputDeviceFlag |= HardwareManager::GetInstance().GetOutputDeviceFlag(i);
    }

    if (outputDeviceFlag & (1 << outputDeviceIndex)) {
        mainSend = fnd::Clamp(param.send[0] + SendMax, SendMin, SendMax);

        for (int i{0}; i < AuxBus_Count; ++i) {
            fxSend[i] = fnd::Clamp(param.send[i + 1], SendMin, SendMax);
        }
    }

    for (int bus{0}; bus < DefaultBusCount; ++bus) {
        for (int channelIndex{0}; channelIndex < ChannelIndex_Count; ++channelIndex) {
            float volume{pre.volume[channelIndex] * param.volume};
            volume *= bus == 0 ? mainSend : fxSend[bus - 1];

            mix->channelGain[channelIndex + bus] = volume;
        }
    }
}
#else
void MultiVoice::CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                             const OutputAdditionalParam* const pAdditionalParam,
                             const PreMixVolume& pre) {
    float sendVolume[OutputMix::ChannelCountMax];
    const int BusCount{m_pOutputReceiver->GetBusCount()};
    for (int i{0}; i < BusCount; ++i)
        sendVolume[i] = 0;

    uint32_t outputDeviceFlag{0};
    for (int i{0}; i < OutputLineIndex_Max; ++i) {
        if (m_OutputLineFlag & (1 << i))
            outputDeviceFlag |= HardwareManager::GetInstance().GetOutputDeviceFlag(i);
    }

    if (outputDeviceFlag & (1 << outputDeviceIndex)) {
        for (int i{0}; i < BusCount; ++i) {
            if (i < DefaultBusCount) {
                const float actualSend{i == 0 ? param.send[i] + SendMax : param.send[i]};
                if (m_pOutputReceiver->IsSoundSendClampEnabled(i))
                    sendVolume[i] = fnd::Clamp(actualSend, SendMin, SendMax);
                else
                    sendVolume[i] = std::max(actualSend, 0.0f);

                continue;
            }

            if (pAdditionalParam == nullptr || !pAdditionalParam->IsAdditionalSendEnabled())
                break;

            if (m_pOutputReceiver->IsSoundSendClampEnabled(i))
                sendVolume[i] =
                    fnd::Clamp(pAdditionalParam->TryGetAdditionalSend(i), SendMin, SendMax);
            else
                sendVolume[i] = std::max(pAdditionalParam->TryGetAdditionalSend(i), 0.0f);
        }
    }

    for (int bus{0}; bus < BusCount; ++bus) {
        for (int voiceChannel{0}; voiceChannel < std::min(m_pOutputReceiver->GetChannelCount(),
                                                          (int32_t)ChannelIndex_Count);
             ++voiceChannel) {
            const int MixBufferIndex{
                GetOutputReceiverMixBufferIndex(m_pOutputReceiver, voiceChannel, bus),
            };

            float volume{param.volume * sendVolume[bus] * pre.volume[MixBufferIndex]};

            if (pAdditionalParam != nullptr && pAdditionalParam->IsVolumeThroughModeUsed()) {
                const bool IsThroughBinaryVolume{
                    (pAdditionalParam->TryGetVolumeThroughMode(bus) & 1) == 0,
                };
                if (IsThroughBinaryVolume)
                    volume *= pAdditionalParam->GetBinaryVolume();
            }

            mix->channelGain[MixBufferIndex] = volume;
        }
    }
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

const Voice& MultiVoice::detail_GetSdkVoice(int32_t channelIndex) const {
    return m_Voice[channelIndex];
}

position_t MultiVoice::GetCurrentPlayingSample() const {
    if (!IsActive())
        return 0;

    return m_Voice[0].GetPlayPosition();
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
