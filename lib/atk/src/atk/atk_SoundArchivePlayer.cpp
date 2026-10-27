#include <nn/atk/atk_SoundArchivePlayer.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundSystem.h>

namespace {

const int32_t MemAlignSize{64};
const size_t FileLoadAlignSize{256};

size_t
GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance(const nn::atk::SoundArchive* arc,
                                                               size_t userParamSizePerSound,
                                                               int32_t addonSoundArchiveCount) {
    size_t result{nn::atk::SoundArchivePlayer::GetRequiredMemSize(arc, userParamSizePerSound,
                                                                  addonSoundArchiveCount)};

    const size_t alignedStreamInstanceSize{
        nn::util::align_up(nn::atk::SoundArchivePlayer::GetRequiredStreamInstanceSize(arc),
                           nn::atk::SoundArchivePlayer::BufferAlignSize),
    };
    result -= alignedStreamInstanceSize;

    return result;
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
nn::atk::OutputReceiver* GetOutputReceiver(const nn::atk::detail::StartInfoReader& reader,
                                           nn::atk::OutputReceiver* pDefaultOutputReceiver);
#endif

const nn::atk::SoundArchive*
GetTargetSoundArchive(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                      const char* soundArchiveName);
const nn::atk::SoundDataManager*
GetTargetSoundDataManager(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                          const char* soundArchiveName);

#if NN_WARE_VER > NN_MAKE_VER(4, 0, 0)
size_t GetSoundPlayerInstanceSize(const nn::atk::detail::SoundInstanceConfig& config) {
    size_t result{sizeof(nn::atk::SoundPlayer)};
    const size_t AdditionalParamBufferSize{
        nn::atk::detail::OutputAdditionalParam::GetRequiredMemSize(config),
    };

    if (AdditionalParamBufferSize != 0) {
        result += sizeof(nn::atk::detail::OutputAdditionalParam);
        result += AdditionalParamBufferSize;
    }

    return result;
}

void SetupSoundInstanceConfigForSoundPlayer(nn::atk::detail::SoundInstanceConfig& config) {
    config.isBusMixVolumeEnabled = false;
    config.isVolumeThroughModeEnabled = false;
}
#endif

}  // anonymous namespace

namespace nn::atk {

SoundArchivePlayer::SoundArchivePlayer() = default;

SoundArchivePlayer::~SoundArchivePlayer() = default;

bool SoundArchivePlayer::IsAvailable() const {
    return m_SoundArchiveManager.IsAvailable();
}

bool SoundArchivePlayer::Initialize(const SoundArchive* arc, const SoundDataManager* manager,
                                    void* buffer, size_t size, void* strmBuffer,
                                    size_t strmBufferSize, size_t userParamSizePerSound) {
    InitializeParam param;
    param.pSoundArchive = arc;
    param.pSoundDataManager = manager;
    param.pSetupBuffer = buffer;
    param.setupBufferSize = size;
    param.pStreamBuffer = strmBuffer;
    param.streamBufferSize = strmBufferSize;
    param.userParamSizePerSound = userParamSizePerSound;

    return Initialize(param);
}

void SoundArchivePlayer::Finalize() {
    if (!m_IsInitialized)
        return;

    StopAllSound(0, true);
    DisposeInstances();

    if (m_IsMemoryPoolForStreamInstanceAttached) {
        SoundSystem::DetachMemoryPool(&m_MemoryPoolForStreamInstance);
        m_IsMemoryPoolForStreamInstanceAttached = false;
    }

    if (m_IsMemoryPoolForPlayerHeapAttached) {
        SoundSystem::DetachMemoryPool(&m_MemoryPoolForPlayerHeap);
        m_IsMemoryPoolForPlayerHeapAttached = false;
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_pDefaultOutputReceiver = nullptr;
#endif
    m_IsInitialized = false;
}

size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* arc) {
    return GetRequiredMemSize(arc, 0, 0);
}

size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* arc,
                                              size_t userParamSizePerSound) {
    return GetRequiredMemSize(arc, userParamSizePerSound, 0);
}

size_t SoundArchivePlayer::GetRequiredMemSize(const SoundArchive* arc, size_t userParamSizePerSound,
                                              int32_t addonSoundArchiveCount) {
    size_t size{0};
    {
#if NN_WARE_VER > NN_MAKE_VER(4, 0, 0)
        detail::SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
        SetupSoundInstanceConfigForSoundPlayer(config);
#endif

        uint32_t playerCount{arc->GetPlayerCount()};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size += util::align_up(playerCount * sizeof(SoundPlayer), BufferAlignSize);
#else
        size += util::align_up(playerCount * GetSoundPlayerInstanceSize(config), BufferAlignSize);
#endif
        for (uint32_t playerIndex{0}; playerIndex < playerCount; ++playerIndex) {
            SoundArchive::PlayerInfo playerInfo;
            if (!arc->ReadPlayerInfo(&playerInfo, detail::Util::GetMaskedItemId(
                                                      playerIndex, detail::ItemType_Player)))
                continue;

            if (playerInfo.playerHeapSize != 0) {
                for (int i{0}; i < playerInfo.playableSoundMax; ++i) {
                    size += util::align_up(playerInfo.playerHeapSize, BufferAlignSize);
                    size += BufferAlignSize;
                }
            }
        }
    }

    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (arc->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo)) {
        size +=
            detail::StreamSoundRuntime::GetRequiredMemorySize(soundArchivePlayerInfo, MemAlignSize);

        bool isAdvancedWaveSoundEnabled{soundArchivePlayerInfo.isAdvancedWaveSoundEnabled};

        size += detail::SequenceSoundRuntime::GetRequiredMemorySize(soundArchivePlayerInfo,
                                                                    MemAlignSize);

        if (isAdvancedWaveSoundEnabled)
            size += detail::AdvancedWaveSoundRuntime::GetRequiredMemorySize(soundArchivePlayerInfo,
                                                                            MemAlignSize);
        else
            size += detail::WaveSoundRuntime::GetRequiredMemorySize(soundArchivePlayerInfo,
                                                                    MemAlignSize);
        size += detail::SequenceSoundRuntime::GetRequiredSequenceTrackMemorySize(
            soundArchivePlayerInfo, MemAlignSize);
    }

    if (addonSoundArchiveCount > 0) {
        size_t containersSize{sizeof(detail::AddonSoundArchiveContainer)};
        containersSize *= addonSoundArchiveCount;
        containersSize = util::align_up(containersSize, MemAlignSize);

        size += containersSize;
    }

    if (userParamSizePerSound != 0) {
        uint32_t soundCount{0};
        soundCount += soundArchivePlayerInfo.sequenceSoundCount;
        soundCount += soundArchivePlayerInfo.waveSoundCount;
        soundCount += soundArchivePlayerInfo.streamSoundCount;

        size_t adjustSize{util::align_up(userParamSizePerSound, UserParamBoundary)};
        adjustSize *= soundCount;

        size += adjustSize;
    }

    return size;
}

// NON_MATCHING: this function matches when GetRequiredMemSize(const SoundArchive*, size_t, int)
// is not implemented, but as soon as we implement it, it breaks the match. We can force a match if
// we uninline GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance and only call
// GetRequiredMemSize(const SoundArchive*, size_t, int), but we know
// GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance is used here because of DWARF info
size_t SoundArchivePlayer::GetRequiredMemSize(const InitializeParam& param) {
    if (param.enablePreparingStreamInstanceBufferFromSetupBuffer) {
        return GetRequiredMemSize(param.pSoundArchive, param.userParamSizePerSound,
                                  param.addonSoundArchiveCount);
    }

    return GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance(
        param.pSoundArchive, param.userParamSizePerSound, param.addonSoundArchiveCount);
}

size_t SoundArchivePlayer::GetRequiredStreamInstanceSize(const SoundArchive* pArchive) {
    SoundArchive::SoundArchivePlayerInfo info;
    if (!pArchive->ReadSoundArchivePlayerInfo(&info))
        return 0;

    return detail::StreamSoundRuntime::GetRequiredStreamInstanceSize(info.streamSoundCount);
}

size_t SoundArchivePlayer::GetRequiredStreamBufferSize(const SoundArchive* arc) const {
    return m_StreamSoundRuntime.GetRequiredStreamBufferSize(arc);
}

size_t SoundArchivePlayer::GetRequiredStreamBufferTimes(const SoundArchive* arc) const {
    return m_StreamSoundRuntime.GetRequiredStreamBufferTimes(arc);
}

size_t SoundArchivePlayer::GetRequiredStreamCacheSize(const SoundArchive* arc,
                                                      size_t cacheSizePerSound) {
    return detail::StreamSoundRuntime::GetRequiredStreamCacheSize(arc, cacheSizePerSound);
}

bool SoundArchivePlayer::Initialize(const InitializeParam& param) {
    if (m_IsInitialized)
        return false;

    if (!SoundSystem::IsInitialized())
        return false;

    util::BytePtr setupBuffer{param.pSetupBuffer};
    size_t setupBufferSize{param.setupBufferSize};

    void* streamSoundInstanceBuffer{};
    size_t streamSoundInstanceBufferSize{0};
    if (param.enablePreparingStreamInstanceBufferFromSetupBuffer) {
        streamSoundInstanceBuffer = setupBuffer.Get();
        streamSoundInstanceBufferSize =
            util::align_up(GetRequiredStreamInstanceSize(param.pSoundArchive), BufferAlignSize);
        if (streamSoundInstanceBufferSize != 0) {
            setupBuffer += streamSoundInstanceBufferSize;
            setupBufferSize -= streamSoundInstanceBufferSize;
            SoundSystem::AttachMemoryPool(&m_MemoryPoolForStreamInstance, streamSoundInstanceBuffer,
                                          streamSoundInstanceBufferSize);
            m_IsMemoryPoolForStreamInstanceAttached = true;
        }
    } else {
        streamSoundInstanceBuffer = param.pStreamInstanceBuffer;
        streamSoundInstanceBufferSize = param.streamInstanceBufferSize;
    }

    if (!SetupMram(param.pSoundArchive, setupBuffer.Get(), setupBufferSize,
                   param.userParamSizePerSound, param.addonSoundArchiveCount,
                   streamSoundInstanceBuffer, streamSoundInstanceBufferSize))
        return false;

    if (!m_StreamSoundRuntime.SetupStreamBuffer(param.pSoundArchive, param.pStreamBuffer,
                                                param.streamBufferSize))
        return false;

    m_SoundArchiveManager.Initialize(param.pSoundArchive, param.pSoundDataManager);
    m_AddonSoundArchiveLastAddTick = 0;
    m_SequenceSoundRuntime.SetSoundArchiveManager(&m_SoundArchiveManager);

    if (param.pStreamCacheBuffer != nullptr) {
        bool setupStreamCacheBufferResult{
            m_StreamSoundRuntime.SetupStreamCacheBuffer(
                param.pSoundArchive, param.pStreamCacheBuffer, param.streamCacheSize),
        };
        if (!setupStreamCacheBufferResult)
            return false;
    }

    m_IsInitialized = true;
    return true;
}

bool SoundArchivePlayer::SetupAddonSoundArchiveContainer(int32_t addonSoundArchiveCount,
                                                         void** pOutAllocatedAddr,
                                                         const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % MemAlignSize != 0)
        return false;

    size_t requireSize{
        util::align_up(addonSoundArchiveCount * sizeof(detail::AddonSoundArchiveContainer),
                       MemAlignSize),
    };
    void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return false;

    void* currentAddr{*pOutAllocatedAddr};
    *pOutAllocatedAddr = estimateEndAddr;

    m_pArchiveContainers = static_cast<detail::AddonSoundArchiveContainer*>(currentAddr);
    m_ArchiveContainerCount = addonSoundArchiveCount;

    for (int i{0}; i < m_ArchiveContainerCount; ++i)
        new (&m_pArchiveContainers[i]) detail::AddonSoundArchiveContainer;

    return true;
}

void SoundArchivePlayer::StopAllSound(int32_t fadeFrames, bool isCommandFlushed) {
    for (SoundArchive::ItemId playerId{0}; playerId < m_SoundPlayerCount; ++playerId)
        m_pSoundPlayers[playerId].StopAllSound(fadeFrames);

    if (isCommandFlushed) {
        detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
        uint32_t tag{cmdmgr.FlushCommand(true, true)};
        cmdmgr.WaitCommandReply(tag);
    }
}

void SoundArchivePlayer::DisposeInstances() {
    m_pArchiveContainers = nullptr;
    m_ArchiveContainerCount = 0;
    m_SoundPlayerCount = 0;
    m_pSoundPlayers = nullptr;

    m_SoundArchiveManager.Finalize();
    m_SequenceSoundRuntime.Finalize();
    if (m_IsAdvancedWaveSoundEnabled)
        m_AdvancedWaveSoundRuntime.Finalize();
    else
        m_WaveSoundRuntime.Finalize();
    m_StreamSoundRuntime.Finalize();
}

}  // namespace nn::atk
