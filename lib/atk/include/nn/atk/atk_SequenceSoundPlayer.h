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

        void Initialize();
    };
    static_assert(sizeof(Data) == 0x50);

    struct Arg {
        const SoundArchive* soundArchive;
        const SoundDataManager* soundDataManager;
        SoundPlayer* soundPlayer;
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
        ~DataLoadTask() override;

        Arg m_Arg;
        Data m_Data;
        PlayerHeap* m_pPlayerHeap;
        PlayerHeapDataManager* m_pPlayerHeapDataManager;
        bool m_IsLoadSuccess;
        [[maybe_unused]] uint8_t m_Padding[3];
    };
    static_assert(sizeof(DataLoadTask) == 0x118);

    class FreePlayerHeapTask : public Task {  // 197
    public:
        void Initialize();
        void Execute(TaskProfileLogger& logger) override;
        ~FreePlayerHeapTask() override;

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

class SequenceSoundPlayer : BasicSoundPlayer, DisposeCallback, SoundThread::PlayerCallback {
public:
    enum StartOffsetType {
        StartOffsetType_Tick,
        StartOffsetType_Millisec,
    };

    enum ResState {
        ResState_Invalid,
        ResState_RecvLoadReq,
        ResState_AppendLoadTask,
        ResState_Assigned,
    };

    constexpr static int32_t PlayerVariableCount = 16;
    constexpr static int32_t GlobalVariableCount = 16;
    constexpr static int32_t TrackCountPerPlayer = 16;

    constexpr static uint32_t AllTrackBitFlag = 0x0000FFFF;

    constexpr static int32_t VariableDefaultValue = -1;

    constexpr static int32_t DefaultTimebase = 48;
    constexpr static int32_t DefaultTempo = 120;
    constexpr static uint32_t DefaultSkipIntervalTick = 16 * DefaultTimebase;

    struct ParserPlayerParam {
        uint8_t priority;
        uint8_t timebase;
        uint16_t tempo;
        MoveValue<uint8_t, int16_t> volume;
        NoteOnCallback* callback;
    };
    static_assert(sizeof(ParserPlayerParam) == 0x18);

    struct StartInfo {
        int32_t seqOffset;
        StartOffsetType startOffsetType;
        int32_t startOffset;
        int32_t delayTime;
        int32_t delayCount;
        UpdateType updateType;
    };
    static_assert(sizeof(StartInfo) == 0x18);

    struct PrepareArg {
        void* seqFile;
        void* bankFiles[4];
        void* warcFiles[4];
        bool warcIsIndividuals[4];
        int32_t seqOffset;
        int32_t delayTime;
        int32_t delayCount;
        UpdateType updateType;
    };
    static_assert(sizeof(PrepareArg) == 0x60);

    struct SetupArg {
        SequenceTrackAllocator* trackAllocator;
        uint32_t allocTracks;
        NoteOnCallback* callback;
    };
    static_assert(sizeof(SetupArg) == 0x18);

    static void InitSequenceSoundPlayer();

    SequenceSoundPlayer();
    ~SequenceSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void FinishPlayer();

    void FreeLoader();

    void Setup(const SetupArg& arg);

    void SetPlayerTrack(int32_t trackNo, SequenceTrack* track);

    void ForceTrackMute(uint32_t);

    SequenceTrack* GetPlayerTrack(int32_t trackNo);

    void Start() override;
    void Stop() override;
    void Pause(bool flag) override;
    void Skip(StartOffsetType offsetType, int32_t offset);

    void SetTempoRatio(float tempoRatio);
    void SetPanRange(float panRange);
    void SetChannelPriority(int32_t priority);
    void SetReleasePriorityFix(bool fix);
    void SetSequenceUserprocCallback(SequenceUserProcCallback callback, void* arg);

    void CallSequenceUserprocCallback(uint16_t procId, SequenceTrack* track);

    int16_t* GetVariablePtr(int32_t varNo);

    void GetLocalVariable(int32_t varNo) const;
    static int16_t GetGlobalVariable(int32_t varNo);

    void SetLocalVariable(int32_t varNo, int16_t var);
    static void SetGlobalVariable(int32_t varNo, int16_t var);

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

    void SetTrackTvVolume(uint32_t trackBitFlag, float volume);
    void SetTrackChannelTvMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                       const MixParameter& param);
    void SetTrackTvPan(uint32_t trackBitFlag, float pan);
    void SetTrackTvSurroundPan(uint32_t trackBitFlag, float surroundPan);
    void SetTrackTvMainSend(uint32_t trackBitFlag, float send);
    void SetTrackTvFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

    void InvalidateData(const void* start, const void* end) override;

    SequenceTrack* GetPlayerTrack(int32_t trackNo) const;
    void CloseTrack(int32_t trackNo);

    void UpdateChannelParam();

    int32_t ParseNextTick(bool doNoteOn);

    void Update();

    bool TryAllocLoader();

    void PrepareForPlayerHeap(PrepareArg* arg);

    void SkipTick();
    void UpdateTick();

    Channel* NoteOn(uint8_t bankIndex, const NoteOnInfo& noteOnInfo);

    void Prepare(const PrepareArg& arg);

    void RequestLoad(const StartInfo& info, const SequenceSoundLoader::Arg& arg);

    uint64_t GetProcessTick(const SoundProfile&);

    void PrepareForMidi(const void**, const void**, bool*);

    static void SetSkipIntervalTick(int32_t);
    static int32_t GetSkipIntervalTick();

    void ChannelCallback(Channel* channel);

    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

private:
    bool m_ReleasePriorityFixFlag;
    bool m_IsPrepared;
    float m_PanRange;
    float m_TempoRatio;
    float m_TickFraction;
    uint32_t m_SkipTickCounter;
    float m_SkipTimeCounter;
    int32_t m_DelayCount;
    ParserPlayerParam m_ParserParam;
    SequenceTrackAllocator* m_pSequenceTrackAllocator;
    SequenceUserProcCallback m_SequenceUserprocCallback;
    void* m_pSequenceUserprocCallbackArg;
    SequenceTrack* m_pTracks[TrackCountPerPlayer];
    int16_t m_LocalVariable[PlayerVariableCount];
    uint32_t m_TickCounter;
    WaveArchiveFileReader m_WarcFileReader[4];
    BankFileReader m_BankFileReader[4];
    uint8_t m_ResState;
    bool m_IsInitialized;
    bool m_IsRegisterPlayerCallback;
    uint8_t m_Padding[1];
    StartInfo m_StartInfo;
    SequenceSoundLoaderManager* m_pLoaderManager;
    SequenceSoundLoader* m_pLoader;
    SequenceSoundLoader::Arg m_LoaderArg;
    UpdateType m_UpdateType;

    static int16_t m_GlobalVariable[GlobalVariableCount];
    static int32_t m_SkipIntervalTickPerFrame;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSoundPlayer) == 0x358);
#else
static_assert(sizeof(SequenceSoundPlayer) == 0x368);
#endif

}  // namespace detail::driver
}  // namespace nn::atk
