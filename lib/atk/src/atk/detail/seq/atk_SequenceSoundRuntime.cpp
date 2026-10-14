#include <nn/atk/atk_SequenceSoundRuntime.h>

#include <nn/atk/atk_Bank.h>
#include <nn/atk/atk_SequenceSoundFileReader.h>
#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_SoundRuntimeUtility.h>
#include <nn/atk/atk_SoundSystem.h>

namespace nn::atk::detail {

SequenceSoundRuntime::SequenceSoundRuntime() {
    m_SequenceCallback.Initialize(*this);
}

SequenceSoundRuntime::~SequenceSoundRuntime() {
    m_pSoundArchiveManager = nullptr;
}

bool SequenceSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                      const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 != 0)
        return false;

    {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size_t requireSize{
            util::align_up(SequenceSoundInstanceManager::GetRequiredMemSize(soundCount), 64),
        };
#else
        size_t requireSize{
            util::align_up(SequenceSoundInstanceManager::GetRequiredMemSize(soundCount, config),
                           64),
        };
#endif

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_SequenceSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize)),
        };
#else
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_SequenceSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
        };
#endif
        *pOutAllocatedAddr = estimateEndAddr;
    }

    {
        size_t requireSize{
            util::align_up(driver::SequenceSoundLoaderManager::GetRequiredMemSize(soundCount), 64),
        };

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

        [[maybe_unused]] uint32_t createdSoundLoaderCount{
            static_cast<uint32_t>(
                m_SequenceSoundLoaderManager.Create(*pOutAllocatedAddr, requireSize)),
        };

        *pOutAllocatedAddr = estimateEndAddr;
    }

    auto& list{m_SequenceSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr)
        itr->SetLoaderManager(m_SequenceSoundLoaderManager);

    driver::SoundThread::GetInstance().RegisterSoundFrameCallback(&m_SequenceSoundLoaderManager);
    m_pSequenceTrackAllocator = &m_MmlSequenceTrackAllocator;

    return true;
}

void SequenceSoundRuntime::Finalize() {
    m_pSequenceTrackAllocator = nullptr;

    m_SequenceSoundInstanceManager.Destroy();
    m_SequenceSoundLoaderManager.Destroy();

    driver::SoundThread::GetInstance().UnregisterSoundFrameCallback(&m_SequenceSoundLoaderManager);
    m_MmlSequenceTrackAllocator.Destroy();
}

bool SequenceSoundRuntime::SetupSequenceTrack(int32_t trackCount, void** pOutAllocatedAddr,
                                              const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 != 0)
        return false;

    size_t requireSize{util::align_up(trackCount * (sizeof(driver::SequenceTrack) + 0x8), 64)};

    void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return false;

    [[maybe_unused]] int32_t createdTrackCount{
        m_MmlSequenceTrackAllocator.Create(*pOutAllocatedAddr, requireSize),
    };
    *pOutAllocatedAddr = estimateEndAddr;
    return true;
}

void SequenceSoundRuntime::SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize) {
    void* curAddr{*pOutAllocatedAddr};

    auto& list{m_SequenceSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr) {
        itr->SetUserParamBuffer(curAddr, adjustSize);
        curAddr = util::BytePtr(curAddr, adjustSize).Get();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    *pOutAllocatedAddr = curAddr;
#endif
}

size_t SequenceSoundRuntime::GetRequiredMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment) {
    size_t size{0};
    size = soundArchivePlayerInfo.sequenceSoundCount;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size = SequenceSoundInstanceManager::GetRequiredMemSize(size);
#else
    size = SequenceSoundInstanceManager::GetRequiredMemSize(size,
                                                            SoundSystem::GetSoundInstanceConfig());
#endif
    size = util::align_up(size, alignment);

    size += util::align_up(driver::SequenceSoundLoaderManager::GetRequiredMemSize(
                               soundArchivePlayerInfo.sequenceSoundCount),
                           alignment);

    return size;
}

size_t SequenceSoundRuntime::GetRequiredSequenceTrackMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment) {
    size_t size{0};
    size = soundArchivePlayerInfo.sequenceTrackCount * (sizeof(driver::SequenceTrack) + 8);
    size = util::align_up(size, alignment);

    return size;
}

bool SequenceSoundRuntime::IsSoundArchiveAvailable() const {
    if (m_pSoundArchiveManager == nullptr)
        return false;

    return m_pSoundArchiveManager->IsAvailable();
}

int32_t SequenceSoundRuntime::GetActiveCount() const {
    return m_SequenceSoundInstanceManager.GetActiveCount();
}

int32_t SequenceSoundRuntime::GetFreeCount() const {
    return m_SequenceSoundInstanceManager.GetFreeCount();
}

void SequenceSoundRuntime::SetSequenceSkipIntervalTick(int32_t intervalTick) {
    driver::SequenceSoundPlayer::SetSkipIntervalTick(intervalTick);
}

int32_t SequenceSoundRuntime::GetSequenceSkipIntervalTick() {
    return driver::SequenceSoundPlayer::GetSkipIntervalTick();
}

void SequenceSoundRuntime::Update() {
    m_SequenceSoundInstanceManager.SortPriorityList();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
SequenceSound* SequenceSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                                int32_t ambientPriority,
                                                BasicSound::AmbientInfo* ambientArgInfo) {
    return SoundRuntimeUtility::AllocSound(&m_SequenceSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo);
}
#else
SequenceSound* SequenceSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                                int32_t ambientPriority,
                                                BasicSound::AmbientInfo* ambientArgInfo,
                                                OutputReceiver* pOutputReceiver) {
    return SoundRuntimeUtility::AllocSound(&m_SequenceSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo, pOutputReceiver);
}
#endif

bool SequenceSoundRuntime::SetupSequenceSoundInfo(
    SoundArchive::SequenceSoundInfo* pOutInfo, SoundArchive::ItemId soundId,
    const SoundArchive& soundArchive,
    const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo) {
    if (!soundArchive.ReadSequenceSoundInfo(pOutInfo, soundId))
        return false;

    if (pExternalSequenceSoundInfo != nullptr) {
        for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
            if (pExternalSequenceSoundInfo->bankIds[i] != InvalidSoundId)
                pOutInfo->bankIds[i] = pExternalSequenceSoundInfo->bankIds[i];
        }
    }

    return true;
}

SoundStartable::StartResult SequenceSoundRuntime::SetupSequenceSoundFile(
    PrepareContext* pOutContext, const SequenceSound& sound, const SoundArchive& soundArchive,
    const SoundDataManager& soundDataManager, const SoundArchive::SoundInfo& soundInfo,
    const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo) {
    if (m_pSoundArchiveFilesHook != nullptr) {
        const char* soundLabel{soundArchive.GetItemLabel(sound.GetId())};
        if (soundLabel != nullptr) {
            pOutContext->pSequenceSoundFile =
                static_cast<const SequenceSoundFile*>(m_pSoundArchiveFilesHook->GetFileAddress(
                    soundLabel, SoundArchiveFilesHook::ItemTypeSequenceSound,
                    SoundArchiveFilesHook::FileTypeSequenceBinary, 0));
        } else {
            pOutContext->pSequenceSoundFile = nullptr;
        }
    }

    if (pOutContext->pSequenceSoundFile == nullptr) {
        pOutContext->pSequenceSoundFile = static_cast<const SequenceSoundFile*>(
            soundDataManager.detail_GetFileAddress(soundInfo.fileId));
        if (pExternalSequenceSoundInfo != nullptr) {
            const void* pExternalSequenceSoundFile{pExternalSequenceSoundInfo->sequenceDataAddress};
            if (pExternalSequenceSoundFile != nullptr) {
                pOutContext->pSequenceSoundFile =
                    static_cast<const SequenceSoundFile*>(pExternalSequenceSoundFile);
            }

            SequenceSoundFileReader fileReader{pOutContext->pSequenceSoundFile};
            const char* startLabel{pExternalSequenceSoundInfo->startLocationLabel};
            if (startLabel != nullptr) {
                if (!fileReader.GetOffsetByLabel(startLabel, &pOutContext->sequenceOffset))
                    return SoundStartable::StartResult::
                        ResultCode_ErrorInvalidSequenceStartLocationLabel;

                pOutContext->sequenceOffset = driver::MmlParser::ParseAllocTrack(
                    fileReader.GetSequenceData(), pOutContext->sequenceOffset,
                    &pOutContext->allocateTrackFlags);
            }
        }
    }

    pOutContext->loadTargetSequenceInfo.itemId = sound.GetId();
    pOutContext->loadTargetSequenceInfo.address = pOutContext->pSequenceSoundFile;

    if (pOutContext->pSequenceSoundFile == nullptr) {
        if (!pOutContext->canUsePlayerHeap)
            return SoundStartable::StartResult::ResultCode_ErrorNotSequenceLoaded;

        pOutContext->isRegisterDataLoadTaskNeeded = true;
    }

    return SoundStartable::StartResult::ResultCode_Success;
}

SoundStartable::StartResult SequenceSoundRuntime::SetupBankFileAndWaveArchiveFile(
    PrepareContext* pOutContext, const SequenceSound& sound,
    const SoundArchive::SequenceSoundInfo& sequenceSoundInfo,
    const SoundArchiveManager::SnapShot& snapShot,
    const SoundStartable::StartInfo::SequenceSoundInfo* pExternalSequenceSoundInfo) {
    if (m_pSoundArchiveFilesHook != nullptr) {
        SetupBankFileAndWaveArchiveFileFromHook(pOutContext, sound,
                                                snapShot.GetCurrentSoundArchive());
    }

    for (int i{0}; static_cast<uint32_t>(i) < SeqBankMax; ++i) {
        if (m_pSoundArchiveFilesHook != nullptr &&
            pOutContext->loadTargetBankInfos[i].address != nullptr &&
            pOutContext->loadTargetWaveArchiveInfos[i].address != nullptr)
            continue;

        SoundArchive::ItemId bankId;
        if (pExternalSequenceSoundInfo != nullptr)
            bankId = pExternalSequenceSoundInfo->bankIds[i];
        else
            bankId = sequenceSoundInfo.bankIds[i];

        const SoundArchive* pSoundArchive{&snapShot.GetCurrentSoundArchive()};
        const SoundDataManager* pSoundDataManager{&snapShot.GetCurrentSoundDataManager()};

        if (bankId != SoundArchive::InvalidId) {
            if (pSoundArchive->IsAddon()) {
                const char* bankName{pSoundArchive->GetItemLabel(bankId)};
                SoundArchive::ItemId mainArchiveBankId{
                    snapShot.GetMainSoundArchive().GetItemId(bankName),
                };

                if (mainArchiveBankId != SoundArchive::InvalidId) {
                    pSoundArchive = &snapShot.GetMainSoundArchive();
                    pSoundDataManager = &snapShot.GetMainSoundDataManager();
                    bankId = mainArchiveBankId;
                }
            }
        }

        SoundArchive::BankInfo bankInfo;
        if (!pSoundArchive->ReadBankInfo(&bankInfo, bankId))
            continue;

        {
            const void* bankFile{pSoundDataManager->detail_GetFileAddress(bankInfo.fileId)};
            pOutContext->loadTargetBankInfos[i].address = bankFile;

            if (bankFile != nullptr) {
                Util::WaveArchiveLoadStatus status{
                    Util::GetWaveArchiveOfBank(pOutContext->loadTargetWaveArchiveInfos[i],
                                               pOutContext->isLoadIndividuals[i], bankFile,
                                               *pSoundArchive, *pSoundDataManager),
                };
                switch (status) {
                case Util::WaveArchiveLoadStatus_Ok:
                case Util::WaveArchiveLoadStatus_Noneed:
                    break;

                case Util::WaveArchiveLoadStatus_Partly:
                    if (pOutContext->canUsePlayerHeap)
                        pOutContext->isRegisterDataLoadTaskNeeded = true;
                    break;

                case Util::WaveArchiveLoadStatus_Error:
                    return SoundStartable::StartResult::ResultCode_ErrorNotBankLoaded;

                case Util::WaveArchiveLoadStatus_NotYet:
                    if (!pOutContext->canUsePlayerHeap)
                        return SoundStartable::StartResult::ResultCode_ErrorNotWarcLoaded;
                    pOutContext->isRegisterDataLoadTaskNeeded = true;
                    break;

                default:
                    return SoundStartable::StartResult::ResultCode_ErrorUnknown;
                }
            } else if (pOutContext->canUsePlayerHeap) {
                pOutContext->loadTargetBankInfos[i].itemId = bankId;
                pOutContext->isRegisterDataLoadTaskNeeded = true;
                return SoundStartable::StartResult::ResultCode_Success;
            }
        }
    }

    return SoundStartable::StartResult::ResultCode_Success;
}

// NON_MATCHING: missing cmp branch
void SequenceSoundRuntime::SetupSequenceSoundPlayerStartInfo(
    driver::SequenceSoundPlayer::StartInfo* pOutStartInfo, uint32_t sequenceOffset,
    const StartInfoReader& startInfoReader) {
    int startOffset;
    driver::SequenceSoundPlayer::StartOffsetType startOffsetType;

    switch (startInfoReader.GetStartOffsetType()) {
    case SoundStartable::StartInfo::StartOffsetType_Sample:
        startOffsetType = driver::SequenceSoundPlayer::StartOffsetType_Tick;
        startOffset = 0;
        break;

    case SoundStartable::StartInfo::StartOffsetType_Tick:
        startOffsetType = driver::SequenceSoundPlayer::StartOffsetType_Tick;
        startOffset = startInfoReader.GetStartOffset();
        break;

    case SoundStartable::StartInfo::StartOffsetType_MilliSeconds:
        startOffsetType = driver::SequenceSoundPlayer::StartOffsetType_Millisec;
        startOffset = startInfoReader.GetStartOffset();
        break;
    }

    pOutStartInfo->seqOffset = sequenceOffset;
    pOutStartInfo->startOffsetType = startOffsetType;
    pOutStartInfo->startOffset = startOffset;
    pOutStartInfo->delayTime = startInfoReader.GetDelayTime();
    pOutStartInfo->delayCount = startInfoReader.GetDelayCount();
    pOutStartInfo->updateType = startInfoReader.GetUpdateType();
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pOutStartInfo->subMixIndex = startInfoReader.GetSubMixIndex();
#endif
}

driver::Channel*
SequenceSoundRuntime::SequenceNoteOnCallback::NoteOn(driver::SequenceSoundPlayer* seqPlayer,
                                                     uint8_t bankIndex,
                                                     const driver::NoteOnInfo& noteOnInfo) {
    if (m_pSequenceSoundRuntime->m_pSoundArchiveManager == nullptr ||
        !m_pSequenceSoundRuntime->m_pSoundArchiveManager->IsAvailable())
        return nullptr;

    detail::BankFileReader& bankReader{seqPlayer->GetBankFileReader(bankIndex)};

    detail::driver::Bank bank;
    driver::Channel* channel{
        bank.NoteOn(bankReader, seqPlayer->GetWaveArchiveFileReader(bankIndex), noteOnInfo),
    };

    return channel;
}

void SequenceSoundRuntime::DumpMemory([[maybe_unused]] const SoundArchive* pSoundArchive) const {}

}  // namespace nn::atk::detail
