#include <nn/atk/atk_SequenceSoundPlayer.h>

#include <nn/atk/atk_DisposeCallbackManager.h>
#include <nn/atk/atk_TaskManager.h>

namespace {

const uint32_t IntervalMsecNumerator{5 << 16};
const uint32_t IntervalMsecDenominator{1 << 16};

}  // anonymous namespace

namespace nn::atk::detail::driver {

void SequenceSoundPlayer::InitSequenceSoundPlayer() {
    for (int variableNo{0}; variableNo < GlobalVariableCount; ++variableNo)
        m_GlobalVariable[variableNo] = VariableDefaultValue;
}

SequenceSoundPlayer::SequenceSoundPlayer() {
    for (int varNo{0}; varNo < PlayerVariableCount; ++varNo)
        m_LocalVariable[varNo] = VariableDefaultValue;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        m_pTracks[trackNo] = nullptr;
}

SequenceSoundPlayer::~SequenceSoundPlayer() {
    Finalize();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void SequenceSoundPlayer::Initialize()
#else
void SequenceSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    BasicSoundPlayer::Initialize();
#else
    BasicSoundPlayer::Initialize(pOutputReceiver);
#endif

    SetPauseFlag(false);
    m_ReleasePriorityFixFlag = false;

    SetStartedFlag(false);
    SetActiveFlag(false);

    m_TickFraction = 0.0f;
    m_SkipTickCounter = 0;
    m_SkipTimeCounter = 0.0f;

    m_PanRange = 1.0f;
    m_TempoRatio = 1.0f;

    m_DelayCount = 0;
    m_TickCounter = 0;

    m_UpdateType = UpdateType_AudioFrame;

    m_SequenceUserprocCallback = nullptr;
    m_pSequenceUserprocCallbackArg = nullptr;

    m_ParserParam.tempo = DefaultTempo;
    m_ParserParam.volume.InitValue(127);
    m_ParserParam.priority = 64;
    m_ParserParam.timebase = DefaultTimebase;
    m_ParserParam.callback = nullptr;

    for (int varNo{0}; varNo < PlayerVariableCount; ++varNo)
        m_LocalVariable[varNo] = VariableDefaultValue;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        m_pTracks[trackNo] = nullptr;

    m_IsInitialized = true;
    m_IsRegisterPlayerCallback = false;
}

void SequenceSoundPlayer::Finalize() {
    SetFinishFlag(true);
    FinishPlayer();

    if (IsActive()) {
        DisposeCallbackManager::GetInstance().UnregisterDisposeCallback(this);
        SetActiveFlag(false);
    }

    for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
        m_BankFileReader[i].Finalize();
        m_WarcFileReader[i].Finalize();
    }

    if (m_IsInitialized) {
        BasicSoundPlayer::Finalize();
        m_IsInitialized = false;
    }

    FreeLoader();
}

void SequenceSoundPlayer::FinishPlayer() {
    if (m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = false;
    }

    if (IsStarted())
        SetStartedFlag(false);

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        CloseTrack(trackNo);
}

void SequenceSoundPlayer::FreeLoader() {
    if (m_pLoader != nullptr) {
        m_pLoaderManager->Free(m_pLoader);
        m_pLoader = nullptr;
    }
}

void SequenceSoundPlayer::Setup(const SetupArg& arg) {
    m_ParserParam.callback = arg.callback;

    {
        int trackCount{0};
        for (uint32_t trackBitMask{arg.allocTracks}; trackBitMask != 0; trackBitMask >>= 1)
            trackCount += trackBitMask & 1;

        int allocatableCount{arg.trackAllocator->GetAllocatableTrackCount()};

        if (trackCount > allocatableCount) {
            Finalize();
            return;
        }
    }

    {
        uint32_t trackBitMask{arg.allocTracks};
        for (int trackNo{0}; trackBitMask != 0; ++trackNo, trackBitMask >>= 1) {
            if (trackBitMask & 1) {
                SequenceTrack* track{arg.trackAllocator->AllocTrack(this)};
                SetPlayerTrack(trackNo, track);
            }
        }
    }

    m_pSequenceTrackAllocator = arg.trackAllocator;
}

void SequenceSoundPlayer::SetPlayerTrack(int32_t trackNo, SequenceTrack* track) {
    if (trackNo > TrackCountPerPlayer - 1)
        return;

    m_pTracks[trackNo] = track;
    track->SetPlayerTrackNo(trackNo);
}

void SequenceSoundPlayer::Start() {
    SetStartedFlag(true);
}

void SequenceSoundPlayer::Stop() {
    FinishPlayer();
}

void SequenceSoundPlayer::Pause(bool flag) {
    SetPauseFlag(flag);

    SequenceTrack* track;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo) {
        track = GetPlayerTrack(trackNo);

        if (track != nullptr)
            track->PauseAllChannel(flag);
    }
}

SequenceTrack* SequenceSoundPlayer::GetPlayerTrack(int32_t trackNo) {
    if (trackNo >= TrackCountPerPlayer)
        return nullptr;

    return m_pTracks[trackNo];
}

void SequenceSoundPlayer::Skip(StartOffsetType offsetType, int32_t offset) {
    if (!IsActive())
        return;

    switch (offsetType) {
    case StartOffsetType_Tick:
        m_SkipTickCounter += offset;
        break;
    case StartOffsetType_Millisec:
        m_SkipTimeCounter += offset;
        break;
    }
}

void SequenceSoundPlayer::SetTempoRatio(float tempoRatio) {
    m_TempoRatio = tempoRatio;
}

void SequenceSoundPlayer::SetPanRange(float panRange) {
    m_PanRange = panRange;
}

void SequenceSoundPlayer::SetChannelPriority(int priority) {
    m_ParserParam.priority = priority;
}

void SequenceSoundPlayer::SetReleasePriorityFix(bool fix) {
    m_ReleasePriorityFixFlag = fix;
}

void SequenceSoundPlayer::SetSequenceUserprocCallback(SequenceUserProcCallback callback,
                                                      void* arg) {
    m_SequenceUserprocCallback = callback;
    m_pSequenceUserprocCallbackArg = arg;
}

void SequenceSoundPlayer::CallSequenceUserprocCallback(uint16_t procId, SequenceTrack* track) {
    if (m_SequenceUserprocCallback == nullptr)
        return;

    SequenceTrack::ParserTrackParam& trackParam{track->GetParserTrackParam()};

    SequenceUserProcCallbackParam param;
    param.localVariable = GetVariablePtr(0);
    param.globalVariable = GetVariablePtr(16);
    param.trackVariable = track->GetVariablePtr(0);
    param.cmpFlag = trackParam.cmpFlag;

    m_SequenceUserprocCallback(procId, &param, m_pSequenceUserprocCallbackArg);

    trackParam.cmpFlag = param.cmpFlag;
}

volatile int16_t* SequenceSoundPlayer::GetVariablePtr(int32_t varNo) {
    if (varNo < PlayerVariableCount)
        return &m_LocalVariable[varNo];

    if (varNo < PlayerVariableCount + GlobalVariableCount)
        return &m_GlobalVariable[varNo - GlobalVariableCount];

    return nullptr;
}

int16_t SequenceSoundPlayer::GetLocalVariable(int32_t varNo) const {
    return m_LocalVariable[varNo];
}

int16_t SequenceSoundPlayer::GetGlobalVariable(int32_t varNo) {
    return m_GlobalVariable[varNo];
}

void SequenceSoundPlayer::SetLocalVariable(int32_t varNo, int16_t var) {
    m_LocalVariable[varNo] = var;
}

void SequenceSoundPlayer::SetGlobalVariable(int32_t varNo, int16_t var) {
    m_GlobalVariable[varNo] = var;
}

void SequenceSoundPlayer::SetTrackMute(uint32_t trackBitFlag, SequenceMute mute) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetMute, mute);
}

void SequenceSoundPlayer::SetTrackSilence(uint64_t trackBitFlag, bool silenceFlag,
                                          int32_t fadeTimes) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetSilence, silenceFlag, fadeTimes);
}

void SequenceSoundPlayer::SetTrackVolume(uint32_t trackBitFlag, float volume) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetVolume, volume);
}

void SequenceSoundPlayer::SetTrackPitch(uint32_t trackBitFlag, float pitch) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetPitch, pitch);
}

void SequenceSoundPlayer::SetTrackLpfFreq(uint32_t trackBitFlag, float lpfFreq) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetLpfFreq, lpfFreq);
}

void SequenceSoundPlayer::SetTrackBiquadFilter(uint32_t trackBitFlag, int32_t type, float value) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetBiquadFilter, type, value);
}

bool SequenceSoundPlayer::SetTrackBankIndex(uint32_t trackBitFlag, int32_t bankIndex) {
    if (!m_BankFileReader[bankIndex].IsInitialized())
        return false;

    SetTrackParam(trackBitFlag, &SequenceTrack::SetBankIndex, bankIndex);
    return true;
}

void SequenceSoundPlayer::SetTrackTranspose(uint32_t trackBitFlag, int8_t transpose) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTranspose, transpose);
}

void SequenceSoundPlayer::SetTrackVelocityRange(uint32_t trackBitFlag, uint8_t range) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetVelocityRange, range);
}

void SequenceSoundPlayer::SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetOutputLine, static_cast<int32_t>(outputLine));
}

void SequenceSoundPlayer::ResetTrackOutputLine(uint32_t trackBitFlag) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetOutputLine, -1);
}

void SequenceSoundPlayer::SetTrackTvVolume(uint32_t trackBitFlag, float volume) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTvVolume, volume);
}

void SequenceSoundPlayer::SetTrackChannelTvMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                                        const MixParameter& param) {
    for (int32_t ch{0}; ch < ChannelIndex_Count; ++ch)
        SetTrackParam(trackBitFlag, &SequenceTrack::SetTvMixParameter, srcChNo, ch, param.ch[ch]);
}

void SequenceSoundPlayer::SetTrackTvPan(uint32_t trackBitFlag, float pan) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTvPan, pan);
}

void SequenceSoundPlayer::SetTrackTvSurroundPan(uint32_t trackBitFlag, float surroundPan) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTvSurroundPan, surroundPan);
}

void SequenceSoundPlayer::SetTrackTvMainSend(uint32_t trackBitFlag, float send) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTvMainSend, send);
}

void SequenceSoundPlayer::SetTrackTvFxSend(uint32_t trackBitFlag, AuxBus bus, float send) {
    SetTrackParam(trackBitFlag, &SequenceTrack::SetTvFxSend, bus, send);
}

void SequenceSoundPlayer::InvalidateData(const void* start, const void* end) {
    if (!IsActive())
        return;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo) {
        SequenceTrack* track{GetPlayerTrack(trackNo)};

        if (track != nullptr) {
            const uint8_t* cur{track->GetParserTrackParam().baseAddr};

            if (start <= cur && cur <= end) {
                Finalize();
                break;
            }
        }
    }

    for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
        const void* cur{m_BankFileReader[i].GetBankFileAddress()};

        if (start <= cur && cur <= end)
            m_BankFileReader[i].Finalize();
    }
}

const SequenceTrack* SequenceSoundPlayer::GetPlayerTrack(int32_t trackNo) const {
    if (trackNo >= TrackCountPerPlayer)
        return nullptr;

    return m_pTracks[trackNo];
}

void SequenceSoundPlayer::CloseTrack(int32_t trackNo) {
    SequenceTrack* track{GetPlayerTrack(trackNo)};

    if (track != nullptr) {
        m_pTracks[trackNo]->Close();
        m_pSequenceTrackAllocator->FreeTrack(m_pTracks[trackNo]);
        m_pTracks[trackNo] = nullptr;
    }
}

void SequenceSoundPlayer::UpdateChannelParam() {
    SequenceTrack* track;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo) {
        track = GetPlayerTrack(trackNo);

        if (track != nullptr)
            track->UpdateChannelParam();
    }
}

int32_t SequenceSoundPlayer::ParseNextTick(bool doNoteOn) {
    m_ParserParam.volume.Update();

    bool activeFlag{false};

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo) {
        SequenceTrack* track{GetPlayerTrack(trackNo)};

        if (track != nullptr) {
            track->UpdateChannelLength();
            if (track->ParseNextTick(doNoteOn) < 0)
                CloseTrack(trackNo);

            activeFlag |= track->IsOpened();
        }
    }

    return !activeFlag;
}

bool SequenceSoundPlayer::TryAllocLoader() {
    if (m_pLoaderManager == nullptr)
        return false;

    SequenceSoundLoader* loader{m_pLoaderManager->Alloc()};
    if (loader == nullptr)
        return false;

    m_pLoader = loader;
    m_ResState = ResState_AppendLoadTask;
    return true;
}

void SequenceSoundLoader::Finalize() {
    m_FreePlayerHeapTask.m_pPlayerHeap = m_Task.m_pPlayerHeap;
    TaskManager::GetInstance().AppendTask(&m_FreePlayerHeapTask, TaskManager::TaskPriority_Middle);
}

bool SequenceSoundLoader::IsInUse() {
    return !m_Task.TryWait() || !m_FreePlayerHeapTask.TryWait();
}

void SequenceSoundLoader::DataLoadTask::Initialize() {
    InitializeStatus();
    m_Data.Initialize();
    m_IsLoadSuccess = false;
}

}  // namespace nn::atk::detail::driver
