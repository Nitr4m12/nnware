#include <nn/atk/atk_WaveSoundRuntime.h>

#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundRuntimeUtility.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/atk_WaveArchiveFileReader.h>

namespace {

const int MemAlignSize{64};

}  // anonymous namespace

namespace nn::atk::detail {

WaveSoundRuntime::WaveSoundRuntime() = default;

WaveSoundRuntime::~WaveSoundRuntime() = default;

bool WaveSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                  const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % MemAlignSize != 0)
        return false;

    {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size_t requireSize{
            util::align_up(WaveSoundInstanceManager::GetRequiredMemSize(soundCount), 64),
        };
#else
        size_t requireSize{
            util::align_up(WaveSoundInstanceManager::GetRequiredMemSize(soundCount, config), 64),
        };
#endif

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(m_WaveSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize)),
        };
#else
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_WaveSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
        };
#endif
        *pOutAllocatedAddr = estimateEndAddr;
    }

    {
        size_t requireSize{driver::WaveSoundLoaderManager::GetRequiredMemSize(soundCount)};

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

        [[maybe_unused]] uint32_t createdSoundLoaderCount{
            static_cast<uint32_t>(m_WaveSoundLoaderManager.Create(*pOutAllocatedAddr, requireSize)),
        };

        *pOutAllocatedAddr = estimateEndAddr;
    }

    auto& list{m_WaveSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr)
        itr->SetLoaderManager(m_WaveSoundLoaderManager);

    driver::SoundThread::GetInstance().RegisterSoundFrameCallback(&m_WaveSoundLoaderManager);

    return true;
}

void WaveSoundRuntime::Finalize() {
    m_WaveSoundInstanceManager.Destroy();
    m_WaveSoundLoaderManager.Destroy();

    driver::SoundThread::GetInstance().UnregisterSoundFrameCallback(&m_WaveSoundLoaderManager);
}

size_t WaveSoundRuntime::GetRequiredMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment) {
    size_t size{0};
    size = soundArchivePlayerInfo.waveSoundCount;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size = WaveSoundInstanceManager::GetRequiredMemSize(size);
#else
    size =
        WaveSoundInstanceManager::GetRequiredMemSize(size, SoundSystem::GetSoundInstanceConfig());
#endif
    size = util::align_up(size, alignment);

    size += util::align_up(
        driver::WaveSoundLoaderManager::GetRequiredMemSize(soundArchivePlayerInfo.waveSoundCount),
        alignment);

    return size;
}

int32_t WaveSoundRuntime::GetActiveCount() const {
    return m_WaveSoundInstanceManager.GetActiveCount();
}

int32_t WaveSoundRuntime::GetFreeWaveSoundCount() const {
    return m_WaveSoundInstanceManager.GetFreeCount();
}

void WaveSoundRuntime::SetupUserParam(void** startAddr, size_t adjustSize) {
    void* curAddr{*startAddr};

    auto& list{m_WaveSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr) {
        itr->SetUserParamBuffer(curAddr, adjustSize);
        curAddr = util::BytePtr(curAddr, adjustSize).Get();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    *startAddr = curAddr;
#endif
}

void WaveSoundRuntime::Update() {
    m_WaveSoundInstanceManager.SortPriorityList();
}
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
WaveSound* WaveSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                        int32_t ambientPriority,
                                        BasicSound::AmbientInfo* ambientArgInfo) {
    return SoundRuntimeUtility::AllocSound(&m_WaveSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo);
}
#else
WaveSound* WaveSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                        int32_t ambientPriority,
                                        BasicSound::AmbientInfo* ambientArgInfo,
                                        OutputReceiver* pOutputReceiver) {
    return SoundRuntimeUtility::AllocSound(&m_WaveSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo, pOutputReceiver);
}
#endif

// NON_MATCHING
SoundStartable::StartResult WaveSoundRuntime::PrepareImpl(const SoundArchive* pSoundArchive,
                                                          const SoundDataManager* pSoundDataManager,
                                                          SoundArchive::ItemId soundId,
                                                          WaveSound* sound,
                                                          const SoundArchive::SoundInfo* commonInfo,
                                                          const StartInfoReader& startInfoReader) {
    SoundArchive::WaveSoundInfo info;
    if (!pSoundArchive->detail_ReadWaveSoundInfo(soundId, &info))
        return SoundStartable::StartResult::ResultCode_ErrorInvalidSoundId;

    const SoundStartable::StartInfo::WaveSoundInfo* externalWsdInfo{startInfoReader.GetWsdInfo()};

    bool canUsePlayerHeap{sound->GetSoundPlayer()->detail_CanUsePlayerHeap()};
    bool isRegisterDataLoadTask{false};
    LoadItemInfo loadWsd{};

    const void* wsdFile{};
    const void* waveFile{};

    if (m_pSoundArchiveFilesHook != nullptr) {
        const char* soundLabel{pSoundArchive->GetItemLabel(sound->GetId())};
        if (soundLabel != nullptr) {
            wsdFile = m_pSoundArchiveFilesHook->GetFileAddress(
                soundLabel, SoundArchiveFilesHook::ItemTypeWaveSound,
                SoundArchiveFilesHook::FileTypeWaveSoundBinary, 0);

            const void* warcFile{
                m_pSoundArchiveFilesHook->GetFileAddress(
                    soundLabel, SoundArchiveFilesHook::ItemTypeWaveSound,
                    SoundArchiveFilesHook::FileTypeWaveArchiveBinary, 0),
            };

            if (wsdFile != nullptr && warcFile != nullptr) {
                WaveArchiveFileReader warcReader{warcFile, false};
                waveFile = warcReader.GetWaveFile(info.index);
                if (waveFile != nullptr) {
                    loadWsd.address = wsdFile;
                }
            }
        }
    }

    int release{127};
    [[maybe_unused]] bool isContextCalculationSkipMode{false};
    int waveSoundParameterFlag{0};

    loadWsd.itemId = sound->GetId();
    if (wsdFile == nullptr || waveFile == nullptr) {
        loadWsd.address = pSoundDataManager->detail_GetFileAddress(commonInfo->fileId);
        if (loadWsd.address != nullptr) {
            wsdFile = loadWsd.address;

            if (externalWsdInfo != nullptr) {
                waveFile = externalWsdInfo->waveAddress;
                waveSoundParameterFlag = externalWsdInfo->enableParameterFlag;
                isContextCalculationSkipMode = externalWsdInfo->isContextCalculationSkipMode;
                release = fnd::Clamp(externalWsdInfo->release, 0, 127);
            }

            if (externalWsdInfo == nullptr || waveFile == nullptr) {
                waveFile = Util::GetWaveFileOfWaveSound(wsdFile, info.index, *pSoundArchive,
                                                        *pSoundDataManager);

                isContextCalculationSkipMode = isContextCalculationSkipMode != false;

                if (waveFile == nullptr) {
                    if (!canUsePlayerHeap)
                        return SoundStartable::StartResult::ResultCode_ErrorNotWarcLoaded;

                    isRegisterDataLoadTask = true;
                }
            }
        } else {
            if (!canUsePlayerHeap)
                return SoundStartable::StartResult::ResultCode_ErrorNotWsdLoaded;

            isRegisterDataLoadTask = true;

            wsdFile = nullptr;
            waveFile = nullptr;
        }
    }

    int startOffset{startInfoReader.GetStartOffset()};
    driver::WaveSoundPlayer::StartOffsetType wsdStartOffsetType;

    switch (startInfoReader.GetStartOffsetType()) {
    case SoundStartable::StartInfo::StartOffsetType_MilliSeconds:
        wsdStartOffsetType = driver::WaveSoundPlayer::StartOffsetType_Millisec;
        break;
    case SoundStartable::StartInfo::StartOffsetType_Tick:
        wsdStartOffsetType = driver::WaveSoundPlayer::StartOffsetType_Sample;
        startOffset = 0;
        break;
    case SoundStartable::StartInfo::StartOffsetType_Sample:
        wsdStartOffsetType = driver::WaveSoundPlayer::StartOffsetType_Sample;
        break;
    default:
        wsdStartOffsetType = driver::WaveSoundPlayer::StartOffsetType_Sample;
        startOffset = 0;
        break;
    }

    driver::WaveSoundPlayer::StartInfo startInfo;
    startInfo.startOffsetType = wsdStartOffsetType;
    startInfo.startOffset = startOffset;
    startInfo.delayTime = startInfoReader.GetDelayTime();
    startInfo.delayCount = startInfoReader.GetDelayCount();
    startInfo.waveSoundParameterFlag = waveSoundParameterFlag;
    startInfo.release = release;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    startInfo.isContextCalculationSkipMode = isContextCalculationSkipMode;
#endif
    startInfo.updateType = startInfoReader.GetUpdateType();

    if (isRegisterDataLoadTask) {
        driver::WaveSoundLoader::LoadInfo loadInfo{pSoundArchive, pSoundDataManager, &loadWsd,
                                                   sound->GetSoundPlayer()};
        sound->RegisterDataLoadTask(loadInfo, startInfo);
    } else {
        int8_t waveType{WaveType_Nwwav};
        if (externalWsdInfo != nullptr && externalWsdInfo->waveAddress != nullptr)
            waveType = externalWsdInfo->waveType;

        sound->Prepare(wsdFile, waveFile, startInfo, waveType);
    }

    SoundStartable::StartResult startResult{SoundStartable::StartResult::ResultCode_Success};
    sound->InitializeChannelParam(info.channelPriority, info.isReleasePriorityFix);

    return startResult;
}

void WaveSoundRuntime::DumpMemory([[maybe_unused]] const SoundArchive* pSoundArchive) const {}

}  // namespace nn::atk::detail
