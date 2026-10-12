#pragma once

#include <nn/atk/atk_MmlSequenceTrackAllocator.h>
#include <nn/atk/atk_SequenceSound.h>
#include <nn/atk/atk_SequenceSoundFile.h>
#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/detail/atk_SoundArchiveManager.h>

namespace nn::atk::detail {

class SequenceSoundRuntime {
public:
    SequenceSoundRuntime();
    ~SequenceSoundRuntime();

    bool Initialize(int32_t soundCount, void** pOutAllocatedAddr, const void* endAddr);
    void Finalize();

    bool SetupSequenceTrack(int32_t trackCount, void** pOutAllocatedAddr, const void* endAddr);
    void SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize);

    static size_t
    GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo,
                          int32_t alignment);
    static size_t GetRequiredSequenceTrackMemorySize(
        const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment);

    void SetSoundArchiveManager(const SoundArchiveManager* manager) {
        m_pSoundArchiveManager = manager;
    }

    void SetSoundArchiveFilesHook(SoundArchiveFilesHook* soundArchiveFilesHook) {
        m_pSoundArchiveFilesHook = soundArchiveFilesHook;
    }

    void SetMmlParser(driver::MmlParser* parser) { m_MmlParser = *parser; }

    void SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* arg) {
        m_SequenceUserProcCallback = callback;
        m_pSequenceUserProcCallbackArg = arg;
    }

    bool IsSoundArchiveAvailable() const;

    int32_t GetActiveCount() const;
    int32_t GetFreeCount() const;

    static void SetSequenceSkipIntervalTick(int32_t intervalTick);
    static int32_t GetSequenceSkipIntervalTick();

    void Update();

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    SequenceSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                              int32_t ambientPriority, BasicSound::AmbientInfo* ambientArgInfo);
#else
    SequenceSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                              int32_t ambientPriority, BasicSound::AmbientInfo* ambientArgInfo,
                              OutputReceiver* pOutputReceiver);
#endif

    SoundStartable::StartResult PrepareImpl(const SoundArchiveManager::SnapShot& snapShot,
                                            SoundArchive::ItemId soundId, SequenceSound* sound,
                                            const SoundArchive::SoundInfo* commonInfo,
                                            const StartInfoReader& startInfoReader);

    void DumpMemory(const SoundArchive* pSoundArchive) const;

private:
    class SequenceNoteOnCallback : public driver::NoteOnCallback {
        NN_NO_COPY(SequenceNoteOnCallback);

    public:
        SequenceNoteOnCallback() = default;

        void Initialize(const SequenceSoundRuntime& runtime);

        driver::Channel* NoteOn(driver::SequenceSoundPlayer* seqPlayer, uint8_t bankIndex,
                                const driver::NoteOnInfo& noteOnInfo) override;

    private:
        friend SequenceSoundRuntime;

        const SequenceSoundRuntime* m_pSequenceSoundRuntime;
    };
    static_assert(sizeof(SequenceNoteOnCallback) == 0x10);

    struct PrepareContext {
        const SequenceSoundFile* pSequenceSoundFile;
        uint32_t sequenceOffset;
        uint32_t allocateTrackFlags;
        LoadItemInfo loadTargetSequenceInfo;
        LoadItemInfo loadTargetBankInfos[SeqBankMax];
        LoadItemInfo loadTargetWaveArchiveInfos[SeqBankMax];
        bool isLoadIndividuals[SeqBankMax];
        bool canUsePlayerHeap;
        bool isRegisterDataLoadTaskNeeded;

        void Initialize();
    };
    static_assert(sizeof(PrepareContext) == 0xa8);

    bool SetupSequenceSoundInfo(
        SoundStartable::StartInfo::SequenceSoundInfo* pOutInfo, SoundArchive::ItemId soundId,
        const SoundArchive& soundArchive,
        const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo);

    SoundStartable::StartResult SetupSequenceSoundFile(
        PrepareContext* pOutContext, const SequenceSound& sound, const SoundArchive& soundArchive,
        const SoundDataManager& soundDataManager, const SoundArchive::SoundInfo& commonInfo,
        const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo);

    SoundStartable::StartResult SetupBankFileAndWaveArchiveFile(
        PrepareContext* pOutContext, const SequenceSound& sound,
        const SoundStartable::StartInfo::SequenceSoundInfo& sequenceSoundInfo,
        const SoundArchiveManager::SnapShot& snapShot,
        const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo);

    void SetupBankFileAndWaveArchiveFileFromHook(PrepareContext* pOutContext,
                                                 const SequenceSound& sound,
                                                 SoundArchive* soundArchive);

    void SetupSequenceSoundPlayerStartInfo(SoundStartable::StartInfo* pOutStartInfo,
                                           uint32_t sequenceOffset,
                                           const StartInfoReader& startInfoReader);

    SequenceSoundInstanceManager m_SequenceSoundInstanceManager;
    driver::SequenceSoundLoaderManager m_SequenceSoundLoaderManager;
    driver::SequenceTrackAllocator* m_pSequenceTrackAllocator{};
    driver::MmlSequenceTrackAllocator m_MmlSequenceTrackAllocator{&m_MmlParser};
    driver::MmlParser m_MmlParser;
    SequenceNoteOnCallback m_SequenceCallback;
    SequenceUserProcCallback m_SequenceUserProcCallback{};
    void* m_pSequenceUserProcCallbackArg{};
    const SoundArchiveManager* m_pSoundArchiveManager{};
    SoundArchiveFilesHook* m_pSoundArchiveFilesHook{};
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSoundRuntime) == 0xe0);
#else
static_assert(sizeof(SequenceSoundRuntime) == 0xe8);
#endif

}  // namespace nn::atk::detail
