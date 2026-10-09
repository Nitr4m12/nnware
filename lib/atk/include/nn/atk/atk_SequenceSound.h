#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_Debug.h>
#include <nn/atk/atk_SequenceSoundPlayer.h>
#include <nn/atk/atk_SoundInstanceManager.h>

namespace nn::atk {

class SequenceSoundHandle;

namespace detail {

class SequenceSound;
using SequenceSoundInstanceManager = SoundInstanceManager<SequenceSound>;

class SequenceSound : public BasicSound {
    NN_ATK_RTTI_OVERRIDE(SequenceSound, BasicSound)

public:
    static const int32_t BankIndexMin{0};
    static const int32_t BankIndexMax{3};

    static const int8_t TransposeMin{-64};
    static const int8_t TransposeMax{63};

    static const uint8_t VelocityRangeMin{0};
    static const uint8_t VelocityRangeMax{127};

    explicit SequenceSound(SequenceSoundInstanceManager& manager);
    ~SequenceSound() override;

    void Setup(driver::SequenceTrackAllocator* trackAllocator, uint32_t allocTracks,
               driver::NoteOnCallback* noteOnCallback, int32_t channelPriority,
               bool isReleasePriorityFix, SequenceUserProcCallback userproc, void* userprocArg);

    struct Resource {
        const void* seq;
        const void* banks[4];
        const void* warcs[4];
        bool warcIsIndividuals[4];

        Resource() = default;
    };
    static_assert(sizeof(Resource) == 0x50);

    void Prepare(const Resource& res, const driver::SequenceSoundPlayer::StartInfo& startInfo);

    void RegisterDataLoadTask(const driver::SequenceSoundLoader::LoadInfo& loadInfo,
                              const driver::SequenceSoundPlayer::StartInfo& startInfo);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    bool IsPrepared() const override {
        if (m_IsCalledPrepare)
            return true;

        if (!IsPlayerAvailable())
            return false;

        return m_PlayerInstance.IsPrepared();
    }

    void SetTempoRatio(float tempoRatio);
    void SetChannelPriority(int32_t priority);

    uint32_t GetTick() const;

    void SetTrackMute(uint32_t trackBitFlag, SequenceMute mute);
    void SetTrackMute(uint32_t trackBitFlag, bool muteFlag);
    void SetTrackSilence(uint32_t trackBitFlag, bool silenceFlag, int32_t fadeFrames);

    void SetTrackBiquadFilter(uint32_t trackBitFlag, int32_t type, float value);
    void SetTrackBankIndex(uint32_t trackBitFlag, int32_t bankIndex);

    void SetTrackVolume(uint32_t trackBitFlag, float volume);
    void SetTrackPitch(uint32_t trackBitFlag, float pitch);
    void SetTrackLpfFreq(uint32_t trackBitFlag, float lpfFreq);
    void SetTrackTranspose(uint32_t trackBitFlag, int8_t transpose);
    void SetTrackVelocityRange(uint32_t trackBitFlag, uint8_t velocityRange);
    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine);
    void ResetTrackOutputLine(uint32_t trackBitFlag);
    void SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                     const MixParameter& mixParam);
    void SetTrackMainOutVolume(uint32_t trackBitFlag, float volume);
    void SetTrackPan(uint32_t trackBitFlag, float pan);
    void SetTrackSurroundPan(uint32_t trackBitFlag, float surroundPan);
    void SetTrackMainSend(uint32_t trackBitFlag, float send);
    void SetTrackFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

    bool ReadVariable(int32_t varNo, int16_t* var) const;
    static bool ReadGlobalVariable(int32_t varNo, int16_t* var);
    bool ReadTrackVariable(int32_t trackNo, int32_t varNo, int16_t* var) const;

    void WriteVariable(int32_t varNo, int16_t var);
    static void WriteGlobalVariable(int32_t varNo, int16_t var);
    void WriteTrackVariable(int32_t trackNo, int32_t varNo, int16_t var);

    void SetLoaderManager(driver::SequenceSoundLoaderManager& manager) {
        if (IsPlayerAvailable())
            m_PlayerInstance.SetLoaderManager(&manager);
    }

    DebugSoundType GetSoundType() const { return DebugSoundType_Seqsound; }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsPlayerAvailable())
            return 0;

        return m_PlayerInstance.GetProcessTick(profile);
    }

    util::IntrusiveListNode m_PriorityLink;

private:
    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;
    void OnUpdatePlayerPriority() override;
    void OnUpdateParam() override;
    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override;

    void Skip(driver::SequenceSoundPlayer::StartOffsetType offsetType, int32_t offset);

    SequenceSoundHandle* m_pTempSpecialHandle;
    SequenceSoundInstanceManager& m_Manager;
    bool m_InitializeFlag{false};
    bool m_CanUseTask;
    bool m_IsCalledPrepare{false};
    uint8_t m_Padding[1];
    driver::SequenceSoundPlayer m_PlayerInstance;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSound) == 0x570);
#else
static_assert(sizeof(SequenceSound) == 0x5a0);
#endif

}  // namespace detail
}  // namespace nn::atk
