#include <nn/atk/atk_StreamSoundRuntime.h>

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

size_t StreamSoundRuntime::GetRequiredStreamCacheSize(const SoundArchive *soundArchive, size_t cacheSizePerSound) {
    int strmSoundCount{0};

    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (soundArchive->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo))
        strmSoundCount = soundArchivePlayerInfo.streamSoundCount;

    size_t memSize{strmSoundCount * cacheSizePerSound};
    return memSize;
}

}  // namespace nn::atk::detail
