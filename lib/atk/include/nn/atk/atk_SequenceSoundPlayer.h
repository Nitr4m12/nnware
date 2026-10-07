#pragma once

#include <nn/atk/atk_BankFileReader.h>
#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_LoaderManager.h>
#include <nn/atk/atk_NoteOnCallback.h>
#include <nn/atk/atk_PlayerHeap.h>
#include <nn/atk/atk_SequenceTrackAllocator.h>
#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_Task.h>
#include <nn/atk/atk_WaveArchiveFileReader.h>

namespace nn::atk {

struct SequenceUserProcCallbackParam {
    volatile int16_t* localVariable;
    volatile int16_t* globalVariable;
    volatile int16_t* trackVariable;
    bool cmpFlag;
};
static_assert(sizeof(SequenceUserProcCallbackParam) == 0x20);

using SequenceUserProcCallback = void (*)(uint16_t, SequenceUserProcCallbackParam*, void*);

namespace detail::driver {

class SequenceSoundLoader;
using SequenceSoundLoaderManager = LoaderManager<SequenceSoundLoader>;

class SequenceSoundLoader {
public:
    struct LoadInfo {
        const SoundArchive* soundArchive;
        const SoundDataManager* soundDataManager;
        LoadItemInfo* loadInfoSeq;
        LoadItemInfo* loadInfoBanks[SeqBankMax];
        SoundPlayer* soundPlayer;

        LoadInfo(const SoundArchive* arc, const SoundDataManager* mgr, LoadItemInfo* seq,
                 LoadItemInfo* banks, SoundPlayer* player);
    };
    static_assert(sizeof(LoadInfo) == 0x40);

    struct Data {
        const void* seqFile;
        const void* bankFiles[SeqBankMax];
        const void* warcFiles[SeqBankMax];
        bool warcIsIndividuals[SeqBankMax];

        Data() = default;

        void Initialize() {
            seqFile = {};

            for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
                bankFiles[i] = nullptr;
                warcFiles[i] = nullptr;
                warcIsIndividuals[i] = false;
            }
        }
    };
    static_assert(sizeof(Data) == 0x50);

    struct Arg {
        const SoundArchive* soundArchive{};
        const SoundDataManager* soundDataManager{};
        SoundPlayer* soundPlayer{};
        LoadItemInfo loadInfoSeq;
        LoadItemInfo loadInfoBanks[SeqBankMax];

        Arg() = default;
    };
    static_assert(sizeof(Arg) == 0x68);

    class DataLoadTask : public Task {
    public:
        void Initialize();
        void Execute(TaskProfileLogger& logger) override;
        bool TryAllocPlayerHeap();

        Arg m_Arg;
        Data m_Data;
        PlayerHeap* m_pPlayerHeap;
        PlayerHeapDataManager* m_pPlayerHeapDataManager;
        bool m_IsLoadSuccess;
        [[maybe_unused]] uint8_t m_Padding[3];
    };
    static_assert(sizeof(DataLoadTask) == 0x118);

    class FreePlayerHeapTask : public Task {
    public:
        void Initialize();
        void Execute(TaskProfileLogger& logger) override;

        Arg m_Arg;
        PlayerHeap* m_pPlayerHeap;
        PlayerHeapDataManager* m_pPlayerHeapDataManager;
    };
    static_assert(sizeof(FreePlayerHeapTask) == 0xc0);

    ~SequenceSoundLoader();

    bool IsInUse();

    void Initialize(const Arg& arg);
    void Finalize();

    bool TryWait();

    bool IsLoadSuccess() const { return m_Task.m_IsLoadSuccess; }
    const Data& GetData() const { return m_Task.m_Data; }

private:
    DataLoadTask m_Task;
    FreePlayerHeapTask m_FreePlayerHeapTask;
    PlayerHeapDataManager m_PlayerHeapDataManager;

public:
    util::IntrusiveListNode m_LinkForLoaderManager;
};
static_assert(sizeof(SequenceSoundLoader) == 0x4b0);

class SequenceSoundPlayer : public BasicSoundPlayer,
                            public DisposeCallback,
                            public SoundThread::PlayerCallback {
public:
    static const int32_t PlayerVariableCount{16};
    static const int32_t GlobalVariableCount{16};
    static const int32_t TrackCountPerPlayer{16};

    static const uint32_t AllTrackBitFlag{0x0000FFFF};

    static const int32_t VariableDefaultValue{-1};

    static const int32_t DefaultTimebase{48};
    static const int32_t DefaultTempo{120};
    static const uint32_t DefaultSkipIntervalTick{DefaultTimebase * 16};

    struct ParserPlayerParam {
        uint8_t priority{64};
        uint8_t timebase{DefaultTimebase};
        uint16_t tempo{DefaultTempo};
        MoveValue<uint8_t, int16_t> volume;
        NoteOnCallback* callback{};

        ParserPlayerParam() { volume.InitValue(127); }
    };
    static_assert(sizeof(ParserPlayerParam) == 0x18);

    enum StartOffsetType {
        StartOffsetType_Tick,
        StartOffsetType_Millisec,
    };

    struct StartInfo {
        int32_t seqOffset;
        StartOffsetType startOffsetType;
        int32_t startOffset;
        int32_t delayTime;
        int32_t delayCount;
        UpdateType updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        int32_t subMixIndex;
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static_assert(sizeof(StartInfo) == 0x1c);
#else
    static_assert(sizeof(StartInfo) == 0x18);
#endif

    static void InitSequenceSoundPlayer();

    static void SetSkipIntervalTick(int32_t intervalTick);
    static int32_t GetSkipIntervalTick();

    SequenceSoundPlayer();
    ~SequenceSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void SetLoaderManager(SequenceSoundLoaderManager* manager) { m_pLoaderManager = manager; }

    struct SetupArg {
        SequenceTrackAllocator* trackAllocator;
        uint32_t allocTracks;
        NoteOnCallback* callback;

        SetupArg() = default;
    };
    static_assert(sizeof(SetupArg) == 0x18);

    void Setup(const SetupArg& arg);

    bool IsPrepared() const { return m_IsPrepared; }

    struct PrepareArg {
        const void* seqFile{};
        const void* bankFiles[SeqBankMax];
        const void* warcFiles[SeqBankMax];
        bool warcIsIndividuals[SeqBankMax];
        int32_t seqOffset{0};
        int32_t delayTime{0};
        int32_t delayCount{0};
        UpdateType updateType{UpdateType_AudioFrame};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        int32_t subMixIndex{0};
#endif

        PrepareArg() {
            for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
                bankFiles[i] = nullptr;
                warcFiles[i] = nullptr;
                warcIsIndividuals[i] = false;
            }
        }
    };
    static_assert(sizeof(PrepareArg) == 0x60);

    void Prepare(const PrepareArg& arg);

    void RequestLoad(const StartInfo& info, const SequenceSoundLoader::Arg& arg);

    void ForceTrackMute(uint32_t trackMask);

    void Start() override;
    void Stop() override;
    void Pause(bool flag) override;
    void Skip(StartOffsetType offsetType, int32_t offset);

    Channel* NoteOn(uint8_t bankIndex, const NoteOnInfo& noteOnInfo);

    void SetSequenceUserprocCallback(SequenceUserProcCallback callback, void* arg);
    void CallSequenceUserprocCallback(uint16_t procId, SequenceTrack* track);

    void SetTempoRatio(float tempoRatio);
    void SetPanRange(float panRange);
    void SetChannelPriority(int priority);
    void SetReleasePriorityFix(bool fix);

    float GetTempoRatio() const { return m_TempoRatio; }
    float GetPanRange() const { return m_PanRange; }
    int32_t GetChannelPriority() const { return m_ParserParam.priority; }
    bool IsReleasePriorityFix() const { return m_ReleasePriorityFixFlag; }

    void SetTrackMute(uint32_t trackBitFlag, SequenceMute mute);
    void SetTrackSilence(uint64_t trackBitFlag, bool silenceFlag, int32_t fadeTimes);
    void SetTrackVolume(uint32_t trackBitFlag, float volume);
    void SetTrackPitch(uint32_t trackBitFlag, float pitch);
    void SetTrackLpfFreq(uint32_t trackBitFlag, float lpfFreq);
    void SetTrackBiquadFilter(uint32_t trackBitFlag, int32_t type, float value);
    bool SetTrackBankIndex(uint32_t trackBitFlag, int32_t bankIndex);
    void SetTrackTranspose(uint32_t trackBitFlag, int8_t transpose);
    void SetTrackVelocityRange(uint32_t trackBitFlag, uint8_t range);
    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine);
    void ResetTrackOutputLine(uint32_t trackBitFlag);

    BankFileReader& GetBankFileReader(uint8_t bankIndex) { return m_BankFileReader[bankIndex]; }
    WaveArchiveFileReader& GetWaveArchiveFileReader(uint8_t bankIndex) {
        return m_WarcFileReader[bankIndex];
    }

    void SetTrackTvVolume(uint32_t trackBitFlag, float volume);
    void SetTrackChannelTvMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                       const MixParameter& param);
    void SetTrackTvPan(uint32_t trackBitFlag, float pan);
    void SetTrackTvSurroundPan(uint32_t trackBitFlag, float surroundPan);
    void SetTrackTvMainSend(uint32_t trackBitFlag, float send);
    void SetTrackTvFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

    void SetTrackDrcVolume(uint32_t, uint32_t, float volume);
    void SetTrackChannelDrcMixParameter(uint32_t, uint32_t, uint32_t srcChNo,
                                        const MixParameter& param);
    void SetTrackDrcPan(uint32_t, uint32_t, float pan);
    void SetTrackDrcSurroundPan(uint32_t, uint32_t, float surroundPan);
    void SetTrackDrcMainSend(uint32_t, uint32_t, float send);
    void SetTrackDrcFxSend(uint32_t, uint32_t, AuxBus bus, float send);

    int16_t GetLocalVariable(int32_t varNo) const;
    static int16_t GetGlobalVariable(int32_t varNo);

    void SetLocalVariable(int32_t varNo, int16_t var);
    static void SetGlobalVariable(int32_t varNo, int16_t var);

    volatile int16_t* GetVariablePtr(int32_t varNo);

    void InvalidateData(const void* start, const void* end) override;

    const ParserPlayerParam& GetParserPlayerParam() const { return m_ParserParam; }
    ParserPlayerParam& GetParserPlayerParam() { return m_ParserParam; }

    uint32_t GetTickCounter() const { return m_TickCounter; }
    UpdateType GetUpdateType() const { return m_UpdateType; }

    SequenceTrack* GetPlayerTrack(int32_t trackNo);
    const SequenceTrack* GetPlayerTrack(int32_t trackNo) const;
    void SetPlayerTrack(int32_t trackNo, SequenceTrack* track);

    const SequenceTrackAllocator* GetTrackAllocator() { return m_pSequenceTrackAllocator; }

    void Update();

    virtual void ChannelCallback(Channel* channel);

    os::Tick GetProcessTick(const SoundProfile& profile);

protected:
    void PrepareForMidi(const void** banks, const void** warcs, bool* warcIsIndividuals);

private:
    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

    void PrepareForPlayerHeap(const PrepareArg& arg);

    bool TryAllocLoader();

    void FreeLoader();

    int32_t ParseNextTick(bool doNoteOn);

    void UpdateChannelParam();

    void UpdateTick();
    void SkipTick();

    void CloseTrack(int32_t trackNo);

    void FinishPlayer();

    float CalcTickPerMinute() {
        return (m_ParserParam.timebase * m_ParserParam.tempo) * m_TempoRatio;
    }

    float CalcTickPerMsec() { return CalcTickPerMinute() / (60 * 1000); }

    static volatile int16_t m_GlobalVariable[GlobalVariableCount];
    static volatile int32_t m_SkipIntervalTickPerFrame;

    bool m_ReleasePriorityFixFlag{false};
    bool m_IsPrepared;
    float m_PanRange{1.0f};
    float m_TempoRatio{1.0f};
    float m_TickFraction{0.0f};
    uint32_t m_SkipTickCounter;
    float m_SkipTimeCounter{0.0f};
    int32_t m_DelayCount{0};
    ParserPlayerParam m_ParserParam;
    SequenceTrackAllocator* m_pSequenceTrackAllocator;
    SequenceUserProcCallback m_SequenceUserprocCallback{};
    void* m_pSequenceUserprocCallbackArg{};
    SequenceTrack* m_pTracks[TrackCountPerPlayer];
    volatile int16_t m_LocalVariable[PlayerVariableCount];
    volatile uint32_t m_TickCounter{0};
    WaveArchiveFileReader m_WarcFileReader[SeqBankMax];
    BankFileReader m_BankFileReader[SeqBankMax];

    enum ResState {
        ResState_Invalid,
        ResState_RecvLoadReq,
        ResState_AppendLoadTask,
        ResState_Assigned,
    };

    uint8_t m_ResState;
    bool m_IsInitialized{false};
    bool m_IsRegisterPlayerCallback{false};
    uint8_t m_Padding[1];
    StartInfo m_StartInfo;
    SequenceSoundLoaderManager* m_pLoaderManager{};
    SequenceSoundLoader* m_pLoader{};
    SequenceSoundLoader::Arg m_LoaderArg;
    UpdateType m_UpdateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    int32_t m_SubMixIndex;
#endif

    template <typename T>
    void SetTrackParam(uint32_t trackBitFlag, void (SequenceTrack::*func)(T), T param) {
        for (int trackNo{0}; trackBitFlag != 0 && trackNo < TrackCountPerPlayer;
             ++trackNo, trackBitFlag >>= 1) {
            if (trackBitFlag & 1) {
                SequenceTrack* track{GetPlayerTrack(trackNo)};
                if (track != nullptr)
                    (track->*func)(param);
            }
        }
    }

    template <typename T1, typename T2>
    void SetTrackParam(uint32_t trackBitFlag, void (SequenceTrack::*func)(T1, T2), T1 t1, T2 t2) {
        for (int trackNo{0}; trackBitFlag != 0 && trackNo < TrackCountPerPlayer;
             ++trackNo, trackBitFlag >>= 1) {
            if (trackBitFlag & 1) {
                SequenceTrack* track{GetPlayerTrack(trackNo)};
                if (track != nullptr)
                    (track->*func)(t1, t2);
            }
        }
    }

    template <typename T1, typename T2, typename T3>
    void SetTrackParam(uint32_t trackBitFlag, void (SequenceTrack::*func)(T1, T2, T3), T1 t1, T2 t2,
                       T3 t3) {
        for (int trackNo{0}; trackBitFlag != 0 && trackNo < TrackCountPerPlayer;
             ++trackNo, trackBitFlag >>= 1) {
            if (trackBitFlag & 1) {
                SequenceTrack* track{GetPlayerTrack(trackNo)};
                if (track != nullptr)
                    (track->*func)(t1, t2, t3);
            }
        }
    }
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSoundPlayer) == 0x358);
#else
static_assert(sizeof(SequenceSoundPlayer) == 0x368);
#endif

}  // namespace detail::driver
}  // namespace nn::atk
