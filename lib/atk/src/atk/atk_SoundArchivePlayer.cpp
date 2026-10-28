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
    size_t result{
        nn::atk::SoundArchivePlayer::GetRequiredMemSize(arc, userParamSizePerSound,
                                                        addonSoundArchiveCount),
    };

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

bool SoundArchivePlayer::SetupMram(const SoundArchive* pArc, void* buffer, size_t size,
                                   size_t userParamSizePerSound, int32_t addonSoundArchiveCount,
                                   void* streamSoundInstanceBuffer,
                                   size_t streamSoundInstanceBufferSize) {
    if (!util::BytePtr(buffer).IsAligned(BufferAlignSize))
        return false;

    const void* endAddr{util::ConstBytePtr(buffer, size).Get()};

    void* curAddr{buffer};
    if (!SetupSoundPlayer(pArc, &curAddr, endAddr))
        return false;

    SoundArchive::SoundArchivePlayerInfo soundArchivePlayerInfo;
    if (pArc->ReadSoundArchivePlayerInfo(&soundArchivePlayerInfo)) {
        if (!m_StreamSoundRuntime.Initialize(soundArchivePlayerInfo.streamSoundCount, &curAddr,
                                             endAddr, streamSoundInstanceBuffer,
                                             streamSoundInstanceBufferSize))
            return false;

        m_IsAdvancedWaveSoundEnabled = soundArchivePlayerInfo.isAdvancedWaveSoundEnabled;

        if (!m_SequenceSoundRuntime.Initialize(soundArchivePlayerInfo.sequenceSoundCount, &curAddr,
                                               endAddr))
            return false;

        if (m_IsAdvancedWaveSoundEnabled) {
            if (!m_AdvancedWaveSoundRuntime.Initialize(soundArchivePlayerInfo.waveSoundCount,
                                                       &curAddr, endAddr))
                return false;
        } else {
            if (!m_WaveSoundRuntime.Initialize(soundArchivePlayerInfo.waveSoundCount, &curAddr,
                                               endAddr))
                return false;
        }

        if (!m_SequenceSoundRuntime.SetupSequenceTrack(soundArchivePlayerInfo.sequenceTrackCount,
                                                       &curAddr, endAddr))
            return false;
    }

    if (addonSoundArchiveCount > 0) {
        if (!SetupAddonSoundArchiveContainer(addonSoundArchiveCount, &curAddr, endAddr))
            return false;
    }

    if (userParamSizePerSound != 0) {
        if (!SetupUserParamForBasicSound(soundArchivePlayerInfo, &curAddr, endAddr,
                                         userParamSizePerSound))
            return false;
    }

    return true;
}

detail::PlayerHeap* SoundArchivePlayer::CreatePlayerHeap(void** pOutAllocatedAddr,
                                                         const void* endAddr, size_t heapSize) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % BufferAlignSize != 0)
        return nullptr;

    void* estimateEndAddr{
        util::BytePtr(*pOutAllocatedAddr, sizeof(detail::PlayerHeap))
            .AlignUp(BufferAlignSize)
            .Get(),
    };
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return nullptr;

    void* curAddr{*pOutAllocatedAddr};
    *pOutAllocatedAddr = estimateEndAddr;

    detail::PlayerHeap* playerHeap{new (curAddr) detail::PlayerHeap};

    estimateEndAddr =
        util::BytePtr(*pOutAllocatedAddr, util::align_up(heapSize, BufferAlignSize)).Get();
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return nullptr;

    curAddr = *pOutAllocatedAddr;
    *pOutAllocatedAddr = estimateEndAddr;

    bool result{playerHeap->Create(curAddr, heapSize)};
    if (!result)
        return nullptr;

    return playerHeap;
}

bool SoundArchivePlayer::SetupSoundPlayer(const SoundArchive* pArc, void** pOutAllocatedAddr,
                                          const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % BufferAlignSize != 0)
        return false;

    const size_t SoundPlayerSize{sizeof(SoundPlayer)};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    detail::SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
    SetupSoundInstanceConfigForSoundPlayer(config);
    const size_t AdditionalParamBufferSize{
        detail::OutputAdditionalParam::GetRequiredMemSize(config),
    };
    const size_t AdditionalParamSize{
        AdditionalParamBufferSize != 0 ? sizeof(detail::OutputAdditionalParam) : 0,
    };
#endif

    uint32_t playerCount{pArc->GetPlayerCount()};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size_t requireSize{util::align_up(playerCount * sizeof(SoundPlayer), BufferAlignSize)};
#else
    size_t requireSize{
        util::align_up(playerCount * GetSoundPlayerInstanceSize(config), BufferAlignSize),
    };
#endif

    void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return false;

    void* curAddr{*pOutAllocatedAddr};
    *pOutAllocatedAddr = estimateEndAddr;

    m_pSoundPlayers = static_cast<SoundPlayer*>(curAddr);
    m_SoundPlayerCount = playerCount;

    uint8_t* ptr{static_cast<uint8_t*>(curAddr)};
    [[maybe_unused]] uint8_t* additionalParamPtr{ptr + playerCount * SoundPlayerSize};
    for (uint32_t playerIndex{0}; playerIndex < playerCount;
         ++playerIndex, ptr += SoundPlayerSize) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        SoundPlayer* player{new (ptr) SoundPlayer};
#else
        detail::OutputAdditionalParam* pAdditionalParam{};
        if (AdditionalParamBufferSize != 0) {
            pAdditionalParam = new (additionalParamPtr) detail::OutputAdditionalParam;
            additionalParamPtr += AdditionalParamSize;

            void* pAdditionalParamBuffer{additionalParamPtr};
            additionalParamPtr += AdditionalParamBufferSize;

            pAdditionalParam->Initialize(pAdditionalParamBuffer, AdditionalParamBufferSize, config);
        }
        SoundPlayer* player{new (ptr) SoundPlayer(pAdditionalParam)};
#endif

        SoundArchive::PlayerInfo playerInfo;
        if (pArc->ReadPlayerInfo(
                &playerInfo, detail::Util::GetMaskedItemId(playerIndex, detail::ItemType_Player))) {
            player->SetPlayableSoundCount(playerInfo.playableSoundMax);
            if (playerInfo.playerHeapSize != 0) {
                for (int i{0}; i < playerInfo.playableSoundMax; ++i) {
                    detail::PlayerHeap* playerHeap{
                        CreatePlayerHeap(pOutAllocatedAddr, endAddr, playerInfo.playerHeapSize),
                    };
                    if (playerHeap == nullptr)
                        return false;

                    player->detail_AppendPlayerHeap(playerHeap);
                }
                player->detail_SetPlayableSoundLimit(playerInfo.playableSoundMax);
            }
        }
    }

    size_t playerHeapSizeDiff{
        static_cast<size_t>(util::ConstBytePtr(estimateEndAddr).Distance(*pOutAllocatedAddr)),
    };
    if (playerHeapSizeDiff != 0) {
        SoundSystem::AttachMemoryPool(&m_MemoryPoolForPlayerHeap, estimateEndAddr,
                                      playerHeapSizeDiff);
        m_IsMemoryPoolForPlayerHeapAttached = true;
    }

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

bool SoundArchivePlayer::SetupUserParamForBasicSound(
    const SoundArchive::SoundArchivePlayerInfo& info, void** pOutAllocatedAddr, const void* endAddr,
    size_t userParamSizePerSound) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % MemAlignSize != 0)
        return false;

    int32_t soundCount{info.sequenceSoundCount + info.waveSoundCount + info.streamSoundCount};
    size_t adjustSize{util::align_up(userParamSizePerSound, UserParamBoundary)};
    size_t requireSize{adjustSize * soundCount};

    void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};

    void* curAddr{*pOutAllocatedAddr};
    *pOutAllocatedAddr = estimateEndAddr;

    if (m_IsAdvancedWaveSoundEnabled)
        m_AdvancedWaveSoundRuntime.SetupUserParam(&curAddr, adjustSize);
    else
        m_WaveSoundRuntime.SetupUserParam(&curAddr, adjustSize);

    m_SequenceSoundRuntime.SetupUserParam(&curAddr, adjustSize);
    m_StreamSoundRuntime.SetupUserParam(&curAddr, adjustSize);

    m_SoundUserParamSize = adjustSize;

    return util::ConstBytePtr(endAddr).Distance(curAddr) <= 0;
}

void SoundArchivePlayer::Update() {
    detail::Util::WarningLogger::GetInstance().SwapBuffer();
    if (m_IsEnableWarningPrint)
        detail::Util::WarningLogger::GetInstance().Print();

    for (uint32_t playerIndex{0}; playerIndex < m_SoundPlayerCount; ++playerIndex)
        GetSoundPlayer(playerIndex).Update();

    m_SequenceSoundRuntime.Update();

    if (m_IsAdvancedWaveSoundEnabled)
        m_AdvancedWaveSoundRuntime.Update();
    else
        m_WaveSoundRuntime.Update();

    m_StreamSoundRuntime.Update();

    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    cmdmgr.RecvCommandReply();
    cmdmgr.FlushCommand(false, false);
}

const SoundArchive& SoundArchivePlayer::GetSoundArchive() const {
    return *m_SoundArchiveManager.GetMainSoundArchive();
}

const AddonSoundArchive*
SoundArchivePlayer::GetAddonSoundArchive(const char* soundArchiveName) const {
    const SoundArchive* pArchive{m_SoundArchiveManager.GetAddonSoundArchive(soundArchiveName)};

    return static_cast<const AddonSoundArchive*>(pArchive);
}

const AddonSoundArchive* SoundArchivePlayer::GetAddonSoundArchive(int32_t index) const {
    const detail::AddonSoundArchiveContainer* pContainer{
        m_SoundArchiveManager.GetAddonSoundArchiveContainer(index),
    };

    return static_cast<const AddonSoundArchive*>(pContainer->GetSoundArchive());
}

const char* SoundArchivePlayer::GetAddonSoundArchiveName(int32_t index) const {
    const detail::AddonSoundArchiveContainer* pContainer{
        m_SoundArchiveManager.GetAddonSoundArchiveContainer(index),
    };

    return pContainer->GetSoundArchiveName();
}

os::Tick SoundArchivePlayer::GetAddonSoundArchiveAddTick(int32_t index) const {
    const detail::AddonSoundArchiveContainer* pContainer{
        m_SoundArchiveManager.GetAddonSoundArchiveContainer(index),
    };

    return pContainer->GetAddTick();
}

const SoundDataManager*
SoundArchivePlayer::GetAddonSoundDataManager(const char* soundArchiveName) const {
    return m_SoundArchiveManager.GetAddonSoundDataManager(soundArchiveName);
}

SoundPlayer& SoundArchivePlayer::GetSoundPlayer(SoundArchive::ItemId playerId) {
    // XXX: DWARF shows the const version of the function is being called here
    return const_cast<SoundPlayer&>(
        static_cast<const SoundArchivePlayer*>(this)->GetSoundPlayer(playerId));
}

SoundPlayer& SoundArchivePlayer::GetSoundPlayer(const char* pStr) {
    // XXX: DWARF shows the const version of the function is being called here
    return const_cast<SoundPlayer&>(
        static_cast<const SoundArchivePlayer*>(this)->GetSoundPlayer(pStr));
}

const SoundPlayer& SoundArchivePlayer::GetSoundPlayer(SoundArchive::ItemId playerId) const {
    uint32_t itemIndex{detail::Util::GetItemIndex(playerId)};

    return m_pSoundPlayers[itemIndex];
}

const SoundPlayer& SoundArchivePlayer::GetSoundPlayer(const char* pStr) const {
    const SoundArchive* pSoundArchive{m_SoundArchiveManager.GetMainSoundArchive()};

    SoundArchive::ItemId playerId{pSoundArchive->GetItemId(pStr)};

    return GetSoundPlayer(playerId);
}

const void* SoundArchivePlayer::detail_GetFileAddress(SoundArchive::FileId fileId) const {
    const SoundDataManager* pSoundDataManager{m_SoundArchiveManager.GetMainSoundDataManager()};
    if (pSoundDataManager == nullptr)
        return nullptr;

    return pSoundDataManager->detail_GetFileAddress(fileId);
}

void SoundArchivePlayer::AddAddonSoundArchive(const char* name,
                                              const AddonSoundArchive* pSoundArchive,
                                              const SoundDataManager* pSoundDataManager) {
    detail::AddonSoundArchiveContainer* pContainer{};
    for (int i{0}; i < m_ArchiveContainerCount; ++i) {
        if (!m_pArchiveContainers[i].IsActive()) {
            pContainer = &m_pArchiveContainers[i];
            break;
        }
    }

    pContainer->Initialize(name, pSoundArchive, pSoundDataManager);

    AddonSoundArchive* pEditableSoundArchive{const_cast<AddonSoundArchive*>(pSoundArchive)};
    pEditableSoundArchive->detail_SetParametersHook(m_SoundArchiveManager.GetParametersHook());

    m_SoundArchiveManager.Add(*pContainer);

    os::Tick tick{os::GetSystemTick()};
    pContainer->SetAddTick(tick);

    m_AddonSoundArchiveLastAddTick = tick;
}

void SoundArchivePlayer::RemoveAddonSoundArchive(const AddonSoundArchive* pSoundArchive) {
    detail::AddonSoundArchiveContainer* pContainer{};
    for (int i{0}; i < m_ArchiveContainerCount; ++i) {
        if (m_pArchiveContainers[i].IsActive() &&
            m_pArchiveContainers[i].GetSoundArchive() == pSoundArchive) {
            pContainer = &m_pArchiveContainers[i];
            break;
        }
    }

    if (pContainer == nullptr)
        return;

    AddonSoundArchive* pEditableSoundArchive{const_cast<AddonSoundArchive*>(pSoundArchive)};
    pEditableSoundArchive->detail_SetParametersHook(nullptr);

    pContainer->Finalize();
    m_SoundArchiveManager.Remove(*pContainer);
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void SoundArchivePlayer::SetDefaultOutputReceiver(OutputReceiver* pOutputReceiver) {
    m_pDefaultOutputReceiver = pOutputReceiver;
}
#endif

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
