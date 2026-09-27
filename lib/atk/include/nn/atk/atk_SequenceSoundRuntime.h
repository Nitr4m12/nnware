#pragma once

#include <nn/atk/atk_MmlSequenceTrackAllocator.h>
#include <nn/atk/atk_SequenceSound.h>
#include <nn/atk/atk_SequenceSoundFile.h>
#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/detail/atk_SoundArchiveManager.h>

namespace nn::atk::detail {

class SequenceSoundRuntime {
public:
    class SequenceNoteOnCallback : driver::NoteOnCallback {
    public:
        ~SequenceNoteOnCallback() override;

        driver::Channel* NoteOn(driver::SequenceSoundPlayer* seqPlayer, uint8_t bankIndex,
                                const driver::NoteOnInfo& noteOnInfo) override;

    private:
        SequenceSoundRuntime* m_pSequenceSoundRuntime;
    };
    static_assert(sizeof(SequenceNoteOnCallback) == 0x10);

    struct PrepareContext {
        SequenceSoundFile* pSequenceSoundFile;
        uint32_t sequenceOffset;
        uint32_t allocateTrackFlags;
        LoadItemInfo loadTargetSequenceInfo;
        LoadItemInfo loadTargetBankInfos[4];
        LoadItemInfo loadTargetWaveArchiveInfos[4];
        bool isLoadIndividuals[4];
        bool canUsePlayerHeap;
        bool isRegisterDataLoadTaskNeeded;
    };
    static_assert(sizeof(PrepareContext) == 0xa8);

    SequenceSoundRuntime();
    ~SequenceSoundRuntime();

    bool Initialize(int32_t soundCount, void** pOutAllocatedAddr, const void* endAddr);
    void Finalize();

    void SetupSequenceTrack(int32_t trackCount, void** pOutAllocatedAddr, const void* endAddr);
    void SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize);

    static size_t
    GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo,
                          int32_t alignment);
    static size_t GetRequiredSequenceTrackMemorySize(
        const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment);

    bool IsSoundArchiveAvailable() const;

    int32_t GetActiveCount() const;
    int32_t GetFreeCount() const;

    static void SetSequenceSkipIntervalTick(int32_t tick);
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

    SoundStartable::StartResult SetupSequenceSoundInfo(
        SoundStartable::StartInfo::SequenceSoundInfo* sequenceSoundInfo,
        SoundArchive::ItemId soundId, const SoundArchive& soundArchive,
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

    void SetupSequenceSoundPlayerStartInfo(SoundStartable::StartInfo* startInfo,
                                           SoundArchive::ItemId soundId,
                                           const StartInfoReader& startInfoReader);

    void DumpMemory(const SoundArchive*) const;

    void SetupBankFileAndWaveArchiveFileFromHook(PrepareContext* pOutContext, SequenceSound* sound,
                                                 SoundArchive* soundArchive);

private:
    SequenceSoundInstanceManager m_SequenceSoundInstanceManager;
    driver::SequenceSoundLoaderManager m_SequenceSoundLoaderManager;
    driver::SequenceTrackAllocator* m_pSequenceTrackAllocator;
    driver::MmlSequenceTrackAllocator m_MmlSequenceTrackAllocator;
    driver::MmlParser m_MmlParser;
    SequenceNoteOnCallback m_SequenceCallback;
    SequenceUserProcCallback m_SequenceUserProcCallback;
    void* m_pSequenceUserProcCallbackArg;
    SoundArchiveManager* m_pSoundArchiveManager;
    SoundArchiveFilesHook* m_pSoundArchiveFilesHook;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SequenceSoundRuntime) == 0xe0);
#else
static_assert(sizeof(SequenceSoundRuntime) == 0xe8);
#endif

}  // namespace nn::atk::detail
