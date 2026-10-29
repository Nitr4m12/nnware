#include <nn/atk/atk_SoundArchivePlayer.h>

#include <algorithm>

#include <nn/atk/atk_Result.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_SoundActor.h>
#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_SoundArchiveParametersHook.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/atk_WaveFileReader.h>
#include <nn/atk/fnd/io/atkfnd_FileStreamImpl.h>

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
                                           nn::atk::OutputReceiver* pDefaultOutputReceiver) {
    nn::atk::OutputReceiver* pOutputReceiver{};
    nn::atk::detail::driver::HardwareManager& hardwareManager{
        nn::atk::detail::driver::HardwareManager().GetInstance(),
    };

    if (hardwareManager.IsPresetSubMixEnabled()) {
        pOutputReceiver = &hardwareManager.GetSubMix(reader.GetSubMixIndex());
    } else {
        pOutputReceiver = reader.GetOutputReceiver();
        if (pOutputReceiver == nullptr) {
            pOutputReceiver = &hardwareManager.GetFinalMix();
            if (pDefaultOutputReceiver != nullptr)
                pOutputReceiver = pDefaultOutputReceiver;
        }
    }

    return pOutputReceiver;
}
#endif

const nn::atk::SoundArchive*
GetTargetSoundArchive(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                      const char* soundArchiveName) {
    if (soundArchiveName == nullptr)
        return soundArchiveManager.GetMainSoundArchive();

    return soundArchiveManager.GetAddonSoundArchive(soundArchiveName);
}

const nn::atk::SoundDataManager*
GetTargetSoundDataManager(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                          const char* soundArchiveName) {
    if (soundArchiveName == nullptr)
        return soundArchiveManager.GetMainSoundDataManager();

    return soundArchiveManager.GetAddonSoundDataManager(soundArchiveName);
}

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

SoundStartable::StartResult SoundArchivePlayer::detail_SetupSound(SoundHandle* handle,
                                                                  uint32_t soundId, bool holdFlag,
                                                                  const char* soundArchiveName,
                                                                  const StartInfo* startInfo) {
    return detail_SetupSoundImpl(handle, soundId, nullptr, nullptr, holdFlag, soundArchiveName,
                                 startInfo);
}

SoundStartable::StartResult SoundArchivePlayer::detail_SetupSoundImpl(
    SoundHandle* handle, uint32_t soundId, detail::BasicSound::AmbientInfo* ambientArgInfo,
    SoundActor* actor, bool holdFlag, const char* soundArchiveName, const StartInfo* startInfo) {
    m_SoundArchiveManager.ChangeTargetArchive(soundArchiveName);
    const detail::SoundArchiveManager::SnapShot snapShot{m_SoundArchiveManager.GetSnapShot()};
    const SoundArchive& soundArchive{snapShot.GetCurrentSoundArchive()};

    if (!m_SoundArchiveManager.IsAvailable())
        return StartResult::ResultCode_ErrorNotAvailable;

    if (handle->IsAttachedSound())
        handle->DetachSound();

    const char* soundLabel{soundArchive.GetItemLabel(soundId)};
    SoundArchive::SoundType soundType{SoundArchive::SoundType_Invalid};

    volatile bool isHookDisabled{false};

    if (IsSoundArchiveFileHooksEnabled() && soundLabel != nullptr) {
        LockSoundArchiveFileHooks();

        if (m_pSoundArchiveFilesHook->IsTargetItem(soundLabel)) {
            soundType = soundArchive.detail_GetParametersHook()->GetSoundType(soundLabel);

            if (soundArchiveName != nullptr)
                soundId = soundArchive.detail_GetParametersHook()->GetItemId(soundLabel);

            switch (soundType) {
            case SoundArchive::SoundType_Sequence:
                isHookDisabled = !IsSequenceSoundEdited(soundLabel);
                break;
            case SoundArchive::SoundType_Stream:
                isHookDisabled = false;
                break;
            case SoundArchive::SoundType_Wave:
                isHookDisabled = !IsWaveSoundEdited(soundLabel);
                break;
            default:
                break;
            }

            if (isHookDisabled) {
                soundArchive.detail_GetParametersHook()->SetIsEnable(false);
                m_pSoundArchiveFilesHook->SetIsEnable(false);
            }
        } else {
            soundArchive.detail_GetParametersHook()->SetIsEnable(false);
            m_pSoundArchiveFilesHook->SetIsEnable(false);
            isHookDisabled = true;

            soundType = soundArchive.GetSoundType(soundId);
        }
    } else {
        if (soundId == SoundArchive::InvalidId)
            return StartResult::ResultCode_ErrorInvalidLabelString;

        soundType = soundArchive.GetSoundType(soundId);
    }

    SoundArchive::SoundInfo soundInfo;
    if (!soundArchive.ReadSoundInfo(&soundInfo, soundId)) {
        EnableHook(&soundArchive, isHookDisabled);

        StartResult result{StartResult::ResultCode_ErrorInvalidSoundId};
        return result;
    }

    detail::StartInfoReader startInfoReader{soundInfo};
    startInfoReader.Read(startInfo);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* pOutputReceiver{GetOutputReceiver(startInfoReader, m_pDefaultOutputReceiver)};
#endif

    if (soundInfo.singlePlayType != SinglePlayType_None) {
        const StartResult result{
            PreprocessSinglePlay(soundInfo, soundId, GetSoundPlayer(startInfoReader.GetPlayerId())),
        };

        if (!result.IsSuccess()) {
            EnableHook(&soundArchive, isHookDisabled);

            return result;
        }
    }

    int playerPriority{startInfoReader.GetPlayerPriority() - holdFlag};

    int ambientPriority{0};
    if (ambientArgInfo != nullptr)
        ambientPriority = detail::BasicSound::GetAmbientPriority(*ambientArgInfo, soundId);

    int allocPriority{
        detail::fnd::Clamp(playerPriority + ambientPriority, PlayerPriorityMin, PlayerPriorityMax),
    };

    detail::ExternalSoundPlayer* extPlayer{};
    if (actor != nullptr) {
        extPlayer = actor->detail_GetActorPlayer(startInfoReader.GetActorPlayerId());
        if (extPlayer == nullptr) {
            EnableHook(&soundArchive, isHookDisabled);

            return StartResult::ResultCode_ErrorInvalidParameter;
        }
    }

    SoundArchive::ItemId playerId{startInfoReader.GetPlayerId()};
    if (!IsSoundArchiveFileHooksEnabled() && soundArchive.IsAddon()) {
        const char* playerName{soundArchive.GetItemLabel(soundInfo.playerId)};
        playerId = snapShot.GetMainSoundArchive().GetItemId(playerName);
    }

    if (playerId == SoundArchive::InvalidId) {
        EnableHook(&soundArchive, isHookDisabled);

        return StartResult::ResultCode_ErrorInvalidParameter;
    }

    SoundPlayer& player{GetSoundPlayer(playerId)};
    if (!player.detail_CanPlaySound(allocPriority)) {
        EnableHook(&soundArchive, isHookDisabled);

        return StartResult::ResultCode_ErrorLowPriority;
    }

    if (extPlayer != nullptr && !extPlayer->CanPlaySound(allocPriority)) {
        EnableHook(&soundArchive, isHookDisabled);

        return StartResult::ResultCode_ErrorLowPriority;
    }

    detail::BasicSound* sound{};
    detail::SequenceSound* seqSound{};
    detail::StreamSound* strmSound{};
    detail::WaveSound* waveSound{};
    detail::AdvancedWaveSound* advancedWaveSound{};

    switch (soundType) {
    case SoundArchive::SoundType_Sequence:
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        seqSound = m_SequenceSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                     ambientArgInfo);
#else
        seqSound = m_SequenceSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                     ambientArgInfo, pOutputReceiver);
#endif
        if (seqSound == nullptr) {
            detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughSeqsound);

            EnableHook(&soundArchive, isHookDisabled);

            SoundStartable::StartResult result{StartResult::ResultCode_ErrorNotEnoughInstance};
            return result;
        }

        sound = seqSound;
        break;
    case SoundArchive::SoundType_Stream:
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        sound = m_StreamSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                ambientArgInfo);
#else
        sound = m_StreamSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                ambientArgInfo, pOutputReceiver);
#endif
        if (sound == nullptr) {
            detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughStrmsound);

            EnableHook(&soundArchive, isHookDisabled);

            SoundStartable::StartResult result{StartResult::ResultCode_ErrorNotEnoughInstance};
            return result;
        }

        strmSound = static_cast<detail::StreamSound*>(sound);
        break;
    case SoundArchive::SoundType_Wave:
        if (m_IsAdvancedWaveSoundEnabled) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
            sound = m_AdvancedWaveSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                          ambientArgInfo);
#else
            sound = m_AdvancedWaveSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                          ambientArgInfo, pOutputReceiver);
#endif
            if (sound == nullptr) {
                detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughWavesound);

                EnableHook(&soundArchive, isHookDisabled);

                SoundStartable::StartResult result{StartResult::ResultCode_ErrorNotEnoughInstance};
                return result;
            }

            advancedWaveSound = static_cast<detail::AdvancedWaveSound*>(sound);
        } else {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
            sound = m_WaveSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                  ambientArgInfo);
#else
            sound = m_WaveSoundRuntime.AllocSound(soundId, playerPriority, ambientPriority,
                                                  ambientArgInfo, pOutputReceiver);
#endif
            if (sound == nullptr) {
                detail::Debug_GetWarningFlag(DebugWarningFlag_NotEnoughWavesound);

                EnableHook(&soundArchive, isHookDisabled);

                SoundStartable::StartResult result{StartResult::ResultCode_ErrorNotEnoughInstance};
                return result;
            }

            waveSound = static_cast<detail::WaveSound*>(sound);
        }

        break;
    default: {
        EnableHook(&soundArchive, isHookDisabled);

        return StartResult::ResultCode_ErrorInvalidSoundId;
    }
    }

    if (!player.detail_AppendSound(sound)) {
        sound->Finalize();
        EnableHook(&soundArchive, isHookDisabled);

        SoundStartable::StartResult result{StartResult::ResultCode_ErrorUnknown};
        return result;
    }

    if (startInfo != nullptr &&
        (startInfo->enableFlag & StartInfo::EnableFlagBit_SoundStopCallback))
        sound->SetSoundStopCallback(startInfo->soundStopCallback);

    sound->ResetOutputLine();

    switch (soundType) {
    case SoundArchive::SoundType_Sequence: {
        SoundStartable::StartResult result{
            m_SequenceSoundRuntime.PrepareImpl(snapShot, soundId, seqSound, &soundInfo,
                                               startInfoReader),
        };
        if (!result.IsSuccess()) {
            seqSound->Finalize();
            EnableHook(&soundArchive, isHookDisabled);

            return result;
        }
        break;
    }
    case SoundArchive::SoundType_Stream: {
        SoundStartable::StartResult result{
            m_StreamSoundRuntime.PrepareImpl(&soundArchive, &snapShot.GetCurrentSoundDataManager(),
                                             soundId, strmSound, &soundInfo, startInfoReader),
        };
        if (!result.IsSuccess()) {
            strmSound->Finalize();
            EnableHook(&soundArchive, isHookDisabled);

            return result;
        }
        break;
    }
    case SoundArchive::SoundType_Wave: {
        if (m_IsAdvancedWaveSoundEnabled) {
            SoundStartable::StartResult result{
                m_AdvancedWaveSoundRuntime.PrepareImpl(
                    &soundArchive, &snapShot.GetCurrentSoundDataManager(), soundId,
                    advancedWaveSound, &soundInfo, startInfoReader),
            };
            if (!result.IsSuccess()) {
                EnableHook(&soundArchive, isHookDisabled);

                return result;
            }
        } else {
            SoundStartable::StartResult result{
                m_WaveSoundRuntime.PrepareImpl(&soundArchive,
                                               &snapShot.GetCurrentSoundDataManager(), soundId,
                                               waveSound, &soundInfo, startInfoReader),
            };
            if (!result.IsSuccess()) {
                waveSound->Finalize();
                EnableHook(&soundArchive, isHookDisabled);

                return result;
            }
        }
        break;
    }
    default: {
        sound->Finalize();
        EnableHook(&soundArchive, isHookDisabled);

        return StartResult::ResultCode_ErrorInvalidSoundId;
    }
    }

    EnableHook(&soundArchive, isHookDisabled);
    SetCommonSoundParam(sound, &soundInfo);

    if (extPlayer != nullptr && !extPlayer->AppendSound(sound)) {
        sound->Finalize();

        return StartResult::ResultCode_ErrorUnknown;
    }

    if (actor != nullptr)
        sound->AttachSoundActor(actor);

    if (holdFlag)
        sound->SetPlayerPriority(startInfoReader.GetPlayerPriority());

    sound->SetSoundArchive(&soundArchive);
    sound->SetSetupTick(os::GetSystemTick());
    handle->detail_AttachSound(sound);

    SoundStartable::StartResult result{StartResult::ResultCode_Success};
    return result;
}

void SoundArchivePlayer::SetCommonSoundParam(detail::BasicSound* sound,
                                             const SoundArchive::SoundInfo* commonInfo) {
    sound->SetInitialVolume(commonInfo->volume / 127.0f);
    sound->SetPanMode(commonInfo->panMode);
    sound->SetPanCurve(commonInfo->panCurve);
}

SoundStartable::StartResult
SoundArchivePlayer::PreprocessSinglePlay(const SoundArchive::SoundInfo& info, uint32_t soundId,
                                         SoundPlayer& player) {
    const os::Tick currentTick{os::GetSystemTick()};
    nn::TimeSpan effectiveDuration{
        nn::TimeSpan::FromMilliSeconds(info.singlePlayEffectiveDuration),
    };

    switch (info.singlePlayType) {
    case SinglePlayType_None:
        break;
    case SinglePlayType_PrioritizeOldest:
        effectiveDuration = 481201171875 << 16;  // Unknown constant
    case SinglePlayType_PrioritizeOldestEffectiveDuration: {
        bool isFound{false};
        os::Tick setupTick{0};

        player.ForEachSound(
            [&isFound, &setupTick, soundId](SoundHandle& handle) -> void {
                if (handle.GetId() == soundId) {
                    setupTick = handle.detail_GetAttachedSound()->GetSetupTick();
                    isFound = true;
                }
            },
            false);

        if (isFound && (currentTick - setupTick).ToTimeSpan() < effectiveDuration)
            return StartResult::ResultCode_CanceledBySinglePlay;

        break;
    }
    case SinglePlayType_PrioritizeNewest:
        effectiveDuration = 481201171875 << 16;  // Unknown constant
    case SinglePlayType_PrioritizeNewestEffectiveDuration:
        player.ForEachSound(
            [soundId, currentTick, effectiveDuration](SoundHandle& handle) -> void {
                if (handle.GetId() == soundId) {
                    if ((currentTick - handle.detail_GetAttachedSound()->GetSetupTick())
                            .ToTimeSpan() < effectiveDuration)
                        handle.Stop(0);
                }
            },
            false);
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }

    return StartResult::ResultCode_Success;
}

void SoundArchivePlayer::EnableHook(const SoundArchive* pSoundArchive, bool isHookDisabled) {
    if (isHookDisabled) {
        pSoundArchive->detail_GetParametersHook()->SetIsEnable(true);
        m_pSoundArchiveFilesHook->SetIsEnable(true);
    }

    UnlockSoundArchiveFileHooks();
}

bool SoundArchivePlayer::IsSoundArchiveFileHooksEnabled() const {
    if (m_pSoundArchiveFilesHook == nullptr)
        return false;

    return m_pSoundArchiveFilesHook->GetIsEnable();
}

void SoundArchivePlayer::LockSoundArchiveFileHooks() {
    if (!IsSoundArchiveFileHooksEnabled())
        return;

    m_pSoundArchiveFilesHook->Lock();
}

void SoundArchivePlayer::UnlockSoundArchiveFileHooks() {
    if (!IsSoundArchiveFileHooksEnabled())
        return;

    return m_pSoundArchiveFilesHook->Unlock();
}

bool SoundArchivePlayer::IsSequenceSoundEdited(const char* soundName) const {
    if (soundName == nullptr)
        return false;

    if (m_pSoundArchiveFilesHook->GetFileAddress(
            soundName, detail::SoundArchiveFilesHook::ItemTypeSequenceSound,
            detail::SoundArchiveFilesHook::FileTypeSequenceBinary, 0) == nullptr)
        return false;

    return true;
}

bool SoundArchivePlayer::IsStreamSoundEdited(const char* soundName) const {
    if (soundName == nullptr)
        return false;

    return true;
}

bool SoundArchivePlayer::IsWaveSoundEdited(const char* soundName) const {
    if (soundName == nullptr)
        return false;

    if (m_pSoundArchiveFilesHook->GetFileAddress(
            soundName, detail::SoundArchiveFilesHook::ItemTypeWaveSound,
            detail::SoundArchiveFilesHook::FileTypeWaveSoundBinary, 0) == nullptr)
        return false;

    return true;
}

void SoundArchivePlayer::SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* arg) {
    m_SequenceSoundRuntime.SetSequenceUserProcCallback(callback, arg);
}

void SoundArchivePlayer::SetSequenceSkipIntervalTick(int32_t intervalTick) {
    detail::SequenceSoundRuntime::SetSequenceSkipIntervalTick(intervalTick);
}

int32_t SoundArchivePlayer::GetSequenceSkipIntervalTick() {
    return detail::SequenceSoundRuntime::GetSequenceSkipIntervalTick();
}

Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                                 SoundArchive::ItemId waveSoundId,
                                                 const SoundArchive* pSoundArchive,
                                                 const SoundDataManager* pSoundDataManager) const {
    SoundArchive::SoundInfo soundInfo;
    if (!pSoundArchive->ReadSoundInfo(&soundInfo, waveSoundId))
        return ResultErrorCAD1();

    SoundArchive::WaveSoundInfo waveSoundInfo;
    if (!pSoundArchive->detail_ReadWaveSoundInfo(waveSoundId, &waveSoundInfo))
        return ResultErrorCAD1();

    const void* pWsdFile{pSoundDataManager->detail_GetFileAddress(soundInfo.fileId)};
    if (pWsdFile == nullptr)
        return ResultErrorD0D1();

    const void* pWaveFile{
        detail::Util::GetWaveFileOfWaveSound(pWsdFile, waveSoundInfo.index, *pSoundArchive,
                                             *pSoundDataManager),
    };
    if (pWaveFile == nullptr)
        return ResultErrorD0D1();

    detail::WaveFileReader waveFileReader{pWaveFile, WaveType_Nwwav};
    detail::WaveInfo waveInfo;
    if (!waveFileReader.ReadWaveInfo(&waveInfo, nullptr))
        return ResultErrorCED1();

    pOutInfo->loopFlag = waveInfo.loopFlag;
    pOutInfo->sampleRate = waveInfo.sampleRate;
    pOutInfo->loopStart = waveInfo.originalLoopStartFrame;
    pOutInfo->loopEnd =
        waveInfo.loopEndFrame - waveInfo.loopStartFrame + waveInfo.originalLoopStartFrame;
    pOutInfo->compatibleLoopStart = waveInfo.loopStartFrame;
    pOutInfo->compatibleLoopEnd = waveInfo.loopEndFrame;
    pOutInfo->channelCount = std::min(waveInfo.channelCount, 2);

    return nn::ResultSuccess();
}

Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                                 SoundArchive::ItemId waveSoundId,
                                                 const char* soundArchiveName) const {
    const SoundArchive* pSoundArchive{
        GetTargetSoundArchive(m_SoundArchiveManager, soundArchiveName),
    };
    const SoundDataManager* pSoundDataManager{
        GetTargetSoundDataManager(m_SoundArchiveManager, soundArchiveName),
    };

    return ReadWaveSoundDataInfo(pOutInfo, waveSoundId, pSoundArchive, pSoundDataManager);
}

Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                                 const char* waveSoundName,
                                                 const char* soundArchiveName) const {
    const SoundArchive* pSoundArchive{
        GetTargetSoundArchive(m_SoundArchiveManager, soundArchiveName),
    };
    const SoundDataManager* pSoundDataManager{
        GetTargetSoundDataManager(m_SoundArchiveManager, soundArchiveName),
    };

    const SoundArchive::ItemId id{pSoundArchive->GetItemId(waveSoundName)};
    if (id == SoundArchive::InvalidId)
        return ResultErrorCCD1();

    return ReadWaveSoundDataInfo(pOutInfo, id, pSoundArchive, pSoundDataManager);
}

Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                                 SoundArchive::ItemId waveSoundId) const {
    return ReadWaveSoundDataInfo(pOutInfo, waveSoundId, nullptr);
}

Result SoundArchivePlayer::ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                                 const char* waveSoundName) const {
    return ReadWaveSoundDataInfo(pOutInfo, waveSoundName, nullptr);
}

// NON_MATCHING: mismatched stack size
Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                                   const SoundArchive* pSoundArchive,
                                                   SoundArchive::ItemId streamSoundId) const {
    bool isStreamSoundPlaying{false};
    for (uint32_t i{0}; i < m_SoundPlayerCount; ++i) {
        m_pSoundPlayers[i].ForEachSound(
            [&isStreamSoundPlaying, streamSoundId](const SoundHandle& handle) -> void {
                if (handle.GetId() == streamSoundId)
                    isStreamSoundPlaying = true;
            },
            false);
    }

    if (isStreamSoundPlaying)
        return ResultErrorD2D1();

    char filePath[detail::FilePathMax + 130];
    if (!pSoundArchive->ReadStreamSoundFilePath(filePath, sizeof(filePath), streamSoundId))
        return ResultErrorD4D1();

    detail::fnd::FileStreamImpl fileStream;
    detail::fnd::FndResult result{
        fileStream.Open(filePath, detail::fnd::FileStream::AccessMode_Read),
    };
    if (result.IsFailed()) {
        fileStream.Close();
        return ResultErrorD8D1();
    }

    const size_t HeaderSize{sizeof(detail::StreamSoundFile::FileHeader) + 0x1c};
    char headerBuffer[HeaderSize];
    size_t readSize{fileStream.Read(headerBuffer, HeaderSize, nullptr)};
    if (readSize != HeaderSize) {
        fileStream.Close();
        return ResultErrorD8D1();
    }

    const auto* header{reinterpret_cast<detail::StreamSoundFile::FileHeader*>(headerBuffer)};
    if (!detail::StreamSoundFileReader::IsValidFileHeader(header)) {
        fileStream.Close();
        return ResultErrorCED1();
    }

    fileStream.Seek(header->GetInfoBlockOffset(), detail::fnd::Stream::SeekOrigin_Begin);

    detail::StreamSoundFile::InfoBlock infoBlock;
    readSize = fileStream.Read(&infoBlock, sizeof(detail::StreamSoundFile::InfoBlock), nullptr);
    if (readSize != sizeof(detail::StreamSoundFile::InfoBlock)) {
        fileStream.Close();
        return ResultErrorD8D1();
    }

    fileStream.Seek(header->GetInfoBlockOffset() + sizeof(detail::BinaryBlockHeader) +
                        infoBlock.body.toStreamSoundInfo.offset,
                    detail::fnd::Stream::SeekOrigin_Begin);

    detail::StreamSoundFile::StreamSoundInfo strmInfo;
    readSize =
        fileStream.Read(&strmInfo, sizeof(detail::StreamSoundFile::StreamSoundInfo), nullptr);

    fileStream.Close();

    if (readSize != sizeof(detail::StreamSoundFile::StreamSoundInfo))
        return ResultErrorD8D1();

    pOutInfo->channelCount = strmInfo.channelCount;
    pOutInfo->sampleRate = strmInfo.sampleRate;
    pOutInfo->loopFlag = strmInfo.isLoop;
    pOutInfo->compatibleLoopStart = strmInfo.loopStart;
    pOutInfo->compatibleLoopEnd = strmInfo.frameCount;
    if (detail::StreamSoundFileReader::IsOriginalLoopAvailableImpl(header)) {
        pOutInfo->loopStart = strmInfo.originalLoopStart;
        pOutInfo->loopEnd = strmInfo.originalLoopEnd;
    } else {
        pOutInfo->loopStart = strmInfo.loopStart;
        pOutInfo->loopEnd = strmInfo.frameCount;
    }

    return nn::ResultSuccess();
}

Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                                   SoundArchive::ItemId streamSoundId,
                                                   const char* soundArchiveName) const {
    const SoundArchive* pSoundArchive{
        GetTargetSoundArchive(m_SoundArchiveManager, soundArchiveName),
    };

    return ReadStreamSoundDataInfo(pOutInfo, pSoundArchive, streamSoundId);
}

Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                                   const char* streamSoundName,
                                                   const char* soundArchiveName) const {
    const SoundArchive* pSoundArchive{
        GetTargetSoundArchive(m_SoundArchiveManager, soundArchiveName),
    };

    const SoundArchive::ItemId id{pSoundArchive->GetItemId(streamSoundName)};
    if (id == SoundArchive::InvalidId)
        return ResultErrorCCD1();

    return ReadStreamSoundDataInfo(pOutInfo, pSoundArchive, id);
}

Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                                   SoundArchive::ItemId streamSoundId) const {
    return ReadStreamSoundDataInfo(pOutInfo, streamSoundId, nullptr);
}

Result SoundArchivePlayer::ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                                   const char* streamSoundName) const {
    return ReadStreamSoundDataInfo(pOutInfo, streamSoundName, nullptr);
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
