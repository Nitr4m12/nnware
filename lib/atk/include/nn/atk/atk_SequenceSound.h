#pragma once

#include <nn/atk/atk_SequenceSoundHandle.h>
#include <nn/atk/atk_SequenceSoundPlayer.h>
#include <nn/atk/atk_SoundInstanceManager.h>

namespace nn::atk::detail {

class SequenceSound;
using SequenceSoundInstanceManager = SoundInstanceManager<SequenceSound>;

class SequenceSound : BasicSound {
public:
    constexpr static uint32_t BankIndexMin = 0;
    constexpr static uint32_t BankIndexMax = 3;

    constexpr static uint8_t TransposeMin = 192;
    constexpr static uint8_t TransposeMax = 63;

    constexpr static uint8_t VelocityRangeMin = 0;
    constexpr static uint32_t VelocityRangeMax = 0x7f00;

    struct Resource {
        void* seq;
        void* banks[4];
        void* warcs[4];
        bool warcIsIndividuals[4];
    };
    static_assert(sizeof(Resource) == 0x50);

    explicit SequenceSound(SequenceSoundInstanceManager& manager);
    ~SequenceSound() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void Setup(driver::SequenceTrackAllocator* trackAllocator, uint32_t allocTracks,
               driver::NoteOnCallback* noteOnCallback, int32_t channelPriority,
               bool isReleasePriorityFix, SequenceUserProcCallback userproc, void* userprocArg);

    void Prepare(const Resource& res, const driver::SequenceSoundPlayer::StartInfo& startInfo);

    void Skip(driver::SequenceSoundPlayer::StartOffsetType, int32_t);

    void SetTempoRatio(float tempoRatio);

    void SetChannelPriority(int32_t priority);
    void OnUpdatePlayerPriority() override;

    void SetTrackMute(uint32_t trackBitFlag, SequenceMute mute);
    void SetTrackMute(uint32_t trackBitFlag, bool);
    void SetTrackSilence(uint32_t trackBitFlag, bool, int32_t);
    void SetTrackVolume(uint32_t trackBitFlag, float volume);
    void SetTrackPitch(uint32_t trackBitFlag, float pitch);
    void SetTrackMainOutVolume(uint32_t trackBitFlag, float volume);
    void SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                     const MixParameter& param);
    void SetTrackPan(uint32_t trackBitFlag, float pan);
    void SetTrackSurroundPan(uint32_t trackBitFlag, float surroundPan);
    void SetTrackMainSend(uint32_t trackBitFlag, float send);
    void SetTrackFxSend(uint32_t trackBitFlag, AuxBus bus, float send);
    void SetTrackLpfFreq(uint32_t trackBitFlag, float lpfFreq);
    void SetTrackBiquadFilter(uint32_t trackBitFlag, int32_t type, float value);
    void SetTrackBankIndex(uint32_t trackBitFlag, int32_t bankIndex);
    void SetTrackTranspose(uint32_t trackBitFlag, int8_t transpose);
    void SetTrackVelocityRange(uint32_t trackBitFlag, uint8_t range);

    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine);
    void ResetTrackOutputLine(uint32_t trackBitFlag);

    bool ReadVariable(int32_t varNo, int16_t* varPtr) const;
    static bool ReadGlobalVariable(int32_t varNo, int16_t* varPtr);
    bool ReadTrackVariable(int32_t, int32_t, int16_t*) const;

    void WriteVariable(int32_t varNo, int16_t var);
    static void WriteGlobalVariable(int32_t varNo, int16_t var);
    void WriteTrackVariable(int32_t, int32_t, int16_t var);

    uint64_t GetTick() const;

    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;

    void RegisterDataLoadTask(const driver::SequenceSoundLoader::LoadInfo& loadInfo,
                              const driver::SequenceSoundPlayer::StartInfo& startInfo);

    bool IsPrepared() const override;

    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override;

    void OnUpdateParam() override;

private:
    friend SequenceSoundInstanceManager;

    util::IntrusiveListNode m_PriorityLink;
    SequenceSoundHandle* m_pTempSpecialHandle;
    SequenceSoundInstanceManager* m_Manager;
    bool m_InitializeFlag;
    bool m_CanUseTask;
    bool m_IsCalledPrepare;
    uint8_t m_Padding[1];
    driver::SequenceSoundPlayer m_PlayerInstance;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSound) == 0x570);
#else
static_assert(sizeof(SequenceSound) == 0x5a0);
#endif

}  // namespace nn::atk::detail
