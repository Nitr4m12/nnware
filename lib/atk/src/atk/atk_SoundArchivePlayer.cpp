#include <nn/atk/atk_SoundArchivePlayer.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundSystem.h>

namespace {

const int32_t MemAlignSize{64};
const size_t FileLoadAlignSize{256};

size_t GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance(
    nn::atk::SoundArchive* arc, size_t userParamSizePerSound, int32_t addonSoundArchiveCount);

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

size_t GetSoundPlayerInstanceSize(const nn::atk::detail::SoundInstanceConfig& config);
void SetupSoundInstanceConfigForSoundPlayer(const nn::atk::detail::SoundInstanceConfig& config);

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

size_t SoundArchivePlayer::GetRequiredStreamInstanceSize(const SoundArchive* pArchive) {
    SoundArchive::SoundArchivePlayerInfo info;
    if (!pArchive->ReadSoundArchivePlayerInfo(&info))
        return 0;

    return detail::StreamSoundRuntime::GetRequiredStreamInstanceSize(info.streamSoundCount);
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
        bool setupStreamCacheBufferResult{m_StreamSoundRuntime.SetupStreamCacheBuffer(
            param.pSoundArchive, param.pStreamCacheBuffer, param.streamCacheSize)};
        if (!setupStreamCacheBufferResult)
            return false;
    }

    m_IsInitialized = true;
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

}  // namespace nn::atk
