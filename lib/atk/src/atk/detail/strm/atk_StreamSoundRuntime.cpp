#include <nn/atk/atk_StreamSoundRuntime.h>

#include <nn/util/util_StringUtil.h>

#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_SoundRuntimeUtility.h>
#include <nn/atk/atk_SoundSystem.h>

namespace {
const int32_t MemAlignSize{64};
}

namespace nn::atk::detail {

StreamSoundRuntime::StreamSoundRuntime() = default;

StreamSoundRuntime::~StreamSoundRuntime() = default;

bool StreamSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                    const void* endAddr, void* streamInstanceBuffer,
                                    [[maybe_unused]] size_t streamInstanceBufferSize) {
    if (!util::is_aligned(reinterpret_cast<uintptr_t>(streamInstanceBuffer), MemAlignSize))
        return false;

    {
        const size_t requireSize{GetRequiredStreamInstanceSize(soundCount)};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        [[maybe_unused]] const uint32_t createdSoundCount{
            static_cast<uint32_t>(
                m_StreamSoundInstanceManager.Create(streamInstanceBuffer, requireSize)),
        };
#else
        [[maybe_unused]] const uint32_t createdSoundCount{
            static_cast<uint32_t>(m_StreamSoundInstanceManager.Create(
                streamInstanceBuffer, requireSize, SoundSystem::GetSoundInstanceConfig())),
        };
#endif
    }

    if (!util::is_aligned(reinterpret_cast<uintptr_t>(*pOutAllocatedAddr), MemAlignSize))
        return false;

    {
        size_t requireSize{
            util::align_up(driver::StreamSoundLoaderManager::GetRequiredMemSize(soundCount),
                           MemAlignSize),
        };

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

        [[maybe_unused]] uint32_t createdSoundLoaderCount{
            static_cast<uint32_t>(
                m_StreamSoundLoaderManager.Create(*pOutAllocatedAddr, requireSize)),
        };

        *pOutAllocatedAddr = estimateEndAddr;
    }

    auto& list{m_StreamSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr)
        itr->SetLoaderManager(m_StreamSoundLoaderManager);

    driver::SoundThread::GetInstance().RegisterSoundFrameCallback(&m_StreamSoundLoaderManager);

    return true;
}

size_t StreamSoundRuntime::GetRequiredStreamInstanceSize(int32_t soundCount) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    return util::align_up(StreamSoundInstanceManager::GetRequiredMemSize(soundCount), MemAlignSize);
#else
    return util::align_up(StreamSoundInstanceManager::GetRequiredMemSize(
                              soundCount, SoundSystem::GetSoundInstanceConfig()),
                          MemAlignSize);
#endif
}

bool StreamSoundRuntime::SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                                           size_t strmBufferSize) {
    return SetupStreamBuffer(pSoundArchive, strmBuffer, strmBufferSize, &m_StreamBufferPool);
}

bool StreamSoundRuntime::SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                                           size_t strmBufferSize,
                                           driver::StreamBufferPool* pStreamBufferPool) {
    GetRequiredStreamBufferSize(pSoundArchive);
    GetRequiredStreamBufferTimes(pSoundArchive);
    if (GetRequiredStreamBufferSize(pSoundArchive) > strmBufferSize)
        return false;

    int strmChannelCount{0};
    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;

    if (pSoundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        strmChannelCount = soundArchivePlayerInfo.streamChannelCount;

    pStreamBufferPool->Initialize(strmBuffer, strmBufferSize, strmChannelCount);
    return true;
}

size_t StreamSoundRuntime::GetRequiredStreamBufferSize(const SoundArchive* soundArchive) const {
    int32_t strmChannelCount{0};

    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (soundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        strmChannelCount = soundArchivePlayerInfo.streamChannelCount;

    size_t memSize{driver::StreamSoundLoader::DataBlockSizeMax};
    memSize *= m_StreamBlockCount;
    memSize *= strmChannelCount;
    return memSize;
}

int32_t StreamSoundRuntime::GetRequiredStreamBufferTimes(const SoundArchive* soundArchive) {
    int strmBufferTimes{1};
    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;

    if (soundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        strmBufferTimes = soundArchivePlayerInfo.streamBufferTimes;

    return strmBufferTimes;
}

bool StreamSoundRuntime::SetupStreamCacheBuffer(const SoundArchive* pSoundArchive,
                                                void* streamCacheBuffer, size_t streamCacheSize) {
    int strmSoundCount{0};
    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (!pSoundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        return false;

    strmSoundCount = soundArchivePlayerInfo.streamSoundCount;
    size_t cacheSize{streamCacheSize / strmSoundCount};

    void* cache{streamCacheBuffer};

    auto& list{m_StreamSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr) {
        itr->SetCacheBuffer(cache, cacheSize);
        cache = util::BytePtr(cache, cacheSize).Get();
    }

    return true;
}

void StreamSoundRuntime::Finalize() {
    m_StreamBufferPool.Finalize();
    m_StreamSoundInstanceManager.Destroy();
    m_StreamSoundLoaderManager.Destroy();

    driver::SoundThread::GetInstance().UnregisterSoundFrameCallback(&m_StreamSoundLoaderManager);
}

size_t StreamSoundRuntime::GetRequiredMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size_t size{
        StreamSoundInstanceManager::GetRequiredMemSize(soundArchivePlayerInfo.streamSoundCount),
    };
#else
    size_t size{
        StreamSoundInstanceManager::GetRequiredMemSize(soundArchivePlayerInfo.streamSoundCount,
                                                       SoundSystem::GetSoundInstanceConfig()),
    };
#endif

    size = util::align_up(size, fnd::Thread::StackAlignment);

    size += util::align_up(driver::StreamSoundLoaderManager::GetRequiredMemSize(
                               soundArchivePlayerInfo.streamSoundCount),
                           alignment);

    return size;
}

size_t StreamSoundRuntime::GetRequiredStreamCacheSize(const SoundArchive* soundArchive,
                                                      size_t cacheSizePerSound) {
    int strmSoundCount{0};

    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (soundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        strmSoundCount = soundArchivePlayerInfo.streamSoundCount;

    size_t memSize{strmSoundCount * cacheSizePerSound};
    return memSize;
}

int32_t StreamSoundRuntime::GetActiveCount() const {
    return m_StreamSoundInstanceManager.GetActiveCount();
}

int32_t StreamSoundRuntime::GetActiveChannelCount() const {
    int count{0};

    auto& activeList{m_StreamSoundInstanceManager.GetSoundList()};
    for (auto iterator{activeList.begin()}; iterator != activeList.end(); ++iterator)
        count += iterator->GetActiveChannelCount();

    return count;
}

int32_t StreamSoundRuntime::GetActiveTrackCount() const {
    int count{0};

    auto& activeList{m_StreamSoundInstanceManager.GetSoundList()};
    for (auto iterator{activeList.begin()}; iterator != activeList.end(); ++iterator)
        count += iterator->GetActiveTrackCount();

    return count;
}

int32_t StreamSoundRuntime::GetFreeCount() const {
    return m_StreamSoundInstanceManager.GetFreeCount();
}

void StreamSoundRuntime::SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize) {
    void* curAddr{*pOutAllocatedAddr};

    auto& list{m_StreamSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr) {
        itr->SetUserParamBuffer(curAddr, adjustSize);
        curAddr = util::BytePtr(curAddr, adjustSize).Get();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    *pOutAllocatedAddr = curAddr;
#endif
}

void StreamSoundRuntime::Update() {
    m_StreamSoundInstanceManager.SortPriorityList();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
StreamSound* StreamSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                            int32_t ambientPriority,
                                            BasicSound::AmbientInfo* ambientArgInfo) {
    return SoundRuntimeUtility::AllocSound(&m_StreamSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo);
}
#else
StreamSound* StreamSoundRuntime::AllocSound(SoundArchive::ItemId soundId, int32_t priority,
                                            int32_t ambientPriority,
                                            BasicSound::AmbientInfo* ambientArgInfo,
                                            OutputReceiver* pOutputReceiver) {
    return SoundRuntimeUtility::AllocSound(&m_StreamSoundInstanceManager, soundId, priority,
                                           ambientPriority, ambientArgInfo, pOutputReceiver);
}
#endif

// NON_MATCHING on versions lower than 4.0.0
SoundStartable::StartResult StreamSoundRuntime::PrepareImpl(
    const SoundArchive* pSoundArchive, const SoundDataManager* pSoundDataManager,
    SoundArchive::ItemId soundId, StreamSound* sound, const SoundArchive::SoundInfo* commonInfo,
    const StartInfoReader& startInfoReader) {
    const SoundArchive::StreamSoundInfo* pStrmMetaInfo{startInfoReader.GetStrmMetaInfo()};
    const SoundArchive::StreamSoundInfo2* pStrmMetaInfo2{startInfoReader.GetStrmMetaInfo2()};

    SoundArchive::StreamSoundInfo info;
    if (pStrmMetaInfo != nullptr) {
        info = *pStrmMetaInfo;
    } else {
        if (!pSoundArchive->ReadStreamSoundInfo(&info, soundId))
            return SoundStartable::StartResult::ResultCode_ErrorInvalidSoundId;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        info.decodeMode = SoundArchive::DecodeMode_Default;
#endif
    }

    const SoundArchive::StreamSoundInfo2* pInfo2{pStrmMetaInfo2};
    SoundArchive::StreamSoundInfo2 info2;
    if (pStrmMetaInfo2 != nullptr) {
        info2 = *pStrmMetaInfo2;
        pInfo2 = &info2;
    } else if (info.streamFileType == SoundArchive::StreamFileType_Opus) {
        if (!pSoundArchive->detail_ReadStreamSoundInfo2(soundId, &info2))
            return SoundStartable::StartResult::ResultCode_ErrorInvalidSoundId;

        pInfo2 = &info2;
    }

    {
        driver::StreamSoundPlayer::SetupArg arg;
        switch (info.streamFileType) {
        case SoundArchive::StreamFileType_NwStreamBinary:
            arg.fileType = StreamFileType_Bfstm;
            break;
        case SoundArchive::StreamFileType_Opus:
            arg.fileType = StreamFileType_Opus;
            break;
        default:
            return SoundStartable::StartResult::ResultCode_ErrorUnknown;
        }

        switch (info.decodeMode) {
        case SoundArchive::DecodeMode_Default:
            arg.decodeMode = DecodeMode_Default;
            break;
        case SoundArchive::DecodeMode_Cpu:
            arg.decodeMode = DecodeMode_Cpu;
            break;
        case SoundArchive::DecodeMode_Accelerator:
            arg.decodeMode = DecodeMode_Accelerator;
            break;
        default:
            return SoundStartable::StartResult::ResultCode_ErrorUnknown;
        }

        for (int i{0}; i < StreamTrackCount; ++i) {
            arg.trackInfos.track[i].volume = info.trackInfo[i].volume;
            arg.trackInfos.track[i].pan = info.trackInfo[i].pan;
            arg.trackInfos.track[i].span = info.trackInfo[i].surroundPan;
            arg.trackInfos.track[i].flags = info.trackInfo[i].flags;
            arg.trackInfos.track[i].mainSend = info.trackInfo[i].mainSend;

            for (int j{0}; j < AuxBus_Count; ++j)
                arg.trackInfos.track[i].fxSend[j] = info.trackInfo[i].fxSend[j];

            arg.trackInfos.track[i].lpfFreq = info.trackInfo[i].lowPassFilterFrequency;
            arg.trackInfos.track[i].biquadType = info.trackInfo[i].biquadType;
            arg.trackInfos.track[i].biquadValue = info.trackInfo[i].biquadValue;
            arg.trackInfos.track[i].channelCount = info.trackInfo[i].channelCount;

            for (int j{0}; j < WaveChannelMax; ++j)
                arg.trackInfos.track[i].channelIndex[j] = info.trackInfo[i].globalChannelIndex[j];
        }

        if (startInfoReader.GetStrmInfo() != nullptr) {
            if (startInfoReader.GetStrmInfo()->pStreamBufferPool != nullptr) {
                arg.pBufferPool = startInfoReader.GetStrmInfo()->pStreamBufferPool;
            }

            if (startInfoReader.GetStrmInfo()->pStreamBufferPool == nullptr) {
                arg.pBufferPool = &m_StreamBufferPool;
            }

        } else {
            arg.pBufferPool = &m_StreamBufferPool;
        }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        arg.loopFlag = false;
        arg.loopStart = 0;
        arg.loopEnd = 0xffffffff;
#endif
        arg.allocChannelCount = info.allocateChannelCount;
        arg.allocTrackFlag = info.allocateTrackFlags;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        arg.loopFlag = false;
        arg.loopStart = 0;
        arg.loopEnd = 0xffffffff;
#endif

        if (pInfo2 != nullptr) {
            arg.loopFlag = pInfo2->isLoop;
            if (arg.loopFlag) {
                arg.loopStart = pInfo2->loopStartFrame;
                arg.loopEnd = pInfo2->loopEndFrame;
            }
        }

        arg.pitch = info.pitch;
        arg.mainSend = info.mainSend;
        for (int i{0}; i < AuxBus_Count; ++i)
            arg.fxSend[i] = info.fxSend[i];

        sound->Setup(arg);
    }

    {
        int startOffset{startInfoReader.GetStartOffset()};
        driver::StreamSoundPlayer::StartOffsetType strmStartOffsetType;
        switch (startInfoReader.GetStartOffsetType()) {
        case SoundStartable::StartInfo::StartOffsetType_MilliSeconds:
            strmStartOffsetType = driver::StreamSoundPlayer::StartOffsetType_Millisec;
            break;
        case SoundStartable::StartInfo::StartOffsetType_Tick:
            startOffset = 0;
            strmStartOffsetType = driver::StreamSoundPlayer::StartOffsetType_Sample;
            break;
        case SoundStartable::StartInfo::StartOffsetType_Sample:
            strmStartOffsetType = driver::StreamSoundPlayer::StartOffsetType_Sample;
            break;
        default:
            startOffset = 0;
            strmStartOffsetType = driver::StreamSoundPlayer::StartOffsetType_Sample;
            break;
        }

        const SoundStartable::StartInfo::StreamSoundInfo* externalStrmInfo{
            startInfoReader.GetStrmInfo(),
        };

        driver::StreamSoundPlayer::PrepareBaseArg arg;

        if (m_pSoundArchiveFilesHook != nullptr) {
            arg.fileStreamHookParam.pSoundArchiveFilesHook = m_pSoundArchiveFilesHook;
            arg.fileStreamHookParam.itemLabel = pSoundArchive->GetItemLabel(sound->GetId());
        }

        arg.startOffsetType = strmStartOffsetType;
        arg.offset = startOffset;
        arg.delayTime = startInfoReader.GetDelayTime();
        arg.delayCount = startInfoReader.GetDelayCount();

        arg.regionCallback =
            externalStrmInfo != nullptr ? externalStrmInfo->regionCallback : nullptr;
        arg.regionCallbackArg =
            externalStrmInfo != nullptr ? externalStrmInfo->regionCallbackArg : nullptr;

        arg.updateType = startInfoReader.GetUpdateType();
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        arg.subMixIndex = startInfoReader.GetSubMixIndex();
#endif

        {
            bool externalPathEnable{false};
            if (externalStrmInfo != nullptr)
                externalPathEnable = externalStrmInfo->externalPath != nullptr;

            if (externalPathEnable) {
                util::Strlcpy(arg.filePath, externalStrmInfo->externalPath, FilePathMax);
            } else {
                if (externalStrmInfo != nullptr && externalStrmInfo->pExternalData != nullptr) {
                    arg.pExternalData = externalStrmInfo->pExternalData;
                    arg.externalDataSize = externalStrmInfo->externalDataSize;
                }

                if (m_pSoundArchiveFilesHook == nullptr || !m_pSoundArchiveFilesHook->GetIsEnable()) {
                    SoundArchive::FileInfo fileInfo;
                    if (!pSoundArchive->detail_ReadFileInfo(commonInfo->fileId, &fileInfo))
                        return SoundStartable::StartResult::ResultCode_ErrorInvalidStreamFileId;

                    if (fileInfo.externalFilePath == nullptr)
                        return SoundStartable::StartResult::ResultCode_ErrorInvalidStreamFilePath;

                    char buf[FilePathMax];
                    pSoundArchive->detail_GetExternalFileFullPath(fileInfo.externalFilePath, buf,
                                                                  sizeof(buf));

                    util::Strlcpy(arg.filePath, buf, sizeof(buf));
                }
            }
        }

        const void* prefetchFile{};

        if (externalStrmInfo != nullptr) {
            prefetchFile = externalStrmInfo->prefetchData;
        }

        if (prefetchFile == nullptr && info.prefetchFileId != SoundArchive::InvalidId) {
            if (m_pSoundArchiveFilesHook != nullptr) {
                const char* soundLabel{pSoundArchive->GetItemLabel(sound->GetId())};
                if (soundLabel != nullptr)
                    prefetchFile = m_pSoundArchiveFilesHook->GetFileAddress(
                        soundLabel, SoundArchiveFilesHook::ItemTypeStreamSound,
                        SoundArchiveFilesHook::FileTypeStreamPrefetchBinary, 0);
            }

            if (prefetchFile == nullptr)
                prefetchFile = pSoundDataManager->detail_GetFileAddress(info.prefetchFileId);
        }

        if (prefetchFile != nullptr && arg.offset == 0)
            sound->PreparePrefetch(prefetchFile, arg);

        sound->Prepare(arg);
    }

    return SoundStartable::StartResult::ResultCode_Success;
}

void StreamSoundRuntime::DumpMemory([[maybe_unused]] const SoundArchive* pSoundArchive) const {}

}  // namespace nn::atk::detail
