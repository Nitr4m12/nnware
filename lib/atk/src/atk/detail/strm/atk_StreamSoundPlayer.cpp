#include <nn/atk/atk_StreamSoundPlayer.h>

#include <algorithm>

#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace {

const uint8_t OpusFileType{nn::atk::detail::StreamFileType_Opus};
const float OpusPitchMax{4.0f};
const uint32_t LoopRegionSizeMin{nn::atk::DataBlockSizeMarginSamples};

}  // anonymous namespace

namespace nn::atk::detail::driver {

uint16_t StreamSoundPlayer::g_AssignNumberCount{};

StreamSoundPlayer::StreamSoundPlayer() = default;

StreamSoundPlayer::~StreamSoundPlayer() {
    Finalize();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void StreamSoundPlayer::Initialize()
#else
void StreamSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    BasicSoundPlayer::Initialize();
#else
    BasicSoundPlayer::Initialize(pOutputReceiver);
    m_LoopCounter = 0;
#endif

    m_PlayingBlockLoopCounter = 0;
    m_PrefetchOffset = 0;
    m_IsPrefetchRevisionCheckEnabled = false;
    m_PrefetchRevisionValue = 0;
    m_DelayCount = 0;
    m_UseDelayCount = false;
    m_LoadFinishFlag = false;
    m_PauseStatus = false;
    m_LoadWaitFlag = false;
    m_IsInitialized = false;
    m_IsPrepared = false;
    m_IsFinalizing = false;
    m_IsPreparedPrefetch = false;
    m_OriginalPlaySamplePosition = 0;
    m_PlaySamplePosition = 0;

    if (TryAllocLoader())
        m_pLoader->Initialize();

    m_ItemData.pitch = 1.0f;
    m_ItemData.mainSend = 1.0f;
    for (int i{0}; i < AuxBus_Count; ++i)
        m_ItemData.fxSend[i] = 0.0f;

    for (int trackIndex{0}; trackIndex < static_cast<int>(StreamTrackCount); ++trackIndex) {
        StreamTrack& track{m_Tracks[trackIndex]};

        track.m_ActiveFlag = false;
        track.m_Volume = 1.0f;
        track.m_OutputLine = -1;
        track.m_TvParam.Initialize();
    }

    for (int channelIndex{0}; channelIndex < StreamChannelCount; ++channelIndex) {
        StreamChannel& channel{m_Channels[channelIndex]};

        channel.m_pBufferAddress = nullptr;
        channel.m_pVoice = nullptr;
    }
}

bool StreamSoundPlayer::TryAllocLoader() {
    if (m_pLoader != nullptr)
        return true;

    if (m_pLoaderManager == nullptr)
        return false;

    StreamSoundLoader* loader{m_pLoaderManager->Alloc()};
    if (loader == nullptr)
        return false;

    m_pLoader = loader;
    return true;
}

void StreamSoundPlayer::Finalize() {
    FinishPlayer();

    if (!m_IsInitialized)
        return;

    m_IsFinalizing = true;
    FreeStreamBuffers();
    FreeVoices();
    FreeLoader();

    m_pBufferPool = nullptr;
    BasicSoundPlayer::Finalize();
    SetActiveFlag(false);
    m_IsInitialized = false;
}

void StreamSoundPlayer::FinishPlayer() {
    if (m_pLoader != nullptr)
        m_pLoader->CancelRequest();

    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        MultiVoice* voice{m_Channels[ch].m_pVoice};
        if (voice != nullptr)
            voice->Stop();
    }

    if (m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = false;
    }

    if (IsStarted())
        SetStartedFlag(false);
}

void StreamSoundPlayer::FreeStreamBuffers() {
    for (int index{0}; index < m_ChannelCount; ++index) {
        if (m_Channels[index].m_pBufferAddress != nullptr) {
            m_pBufferPool->Free(m_Channels[index].m_pBufferAddress);
            m_Channels[index].m_pBufferAddress = nullptr;
        }
    }
}

void StreamSoundPlayer::FreeVoices() {
    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        StreamChannel& channel{m_Channels[ch]};

        if (channel.m_pVoice != nullptr) {
            channel.m_pVoice->Free();
            channel.m_pVoice = nullptr;
        }
    }
}

void StreamSoundPlayer::FreeLoader() {
    if (m_pLoader == nullptr || m_pLoaderManager == nullptr)
        return;

    m_pLoaderManager->Free(m_pLoader);
    m_pLoader = nullptr;
}

void StreamSoundPlayer::Setup(const SetupArg& arg) {
    if (m_pLoader == nullptr) {
        m_SetupArg = arg;
        return;
    }

    m_FileType = arg.fileType;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    m_DecodeMode = arg.decodeMode;
#endif
    m_LoopFlag = arg.loopFlag;
    m_LoopStart = arg.loopStart;
    m_LoopEnd = arg.loopEnd;

    uint16_t assignNumberCount{g_AssignNumberCount};
    g_AssignNumberCount = g_AssignNumberCount + 1;

    m_AssignNumber = assignNumberCount;
    m_pLoader->SetAssignNumber(assignNumberCount);

    m_ItemData.Set(arg);

    if (!SetupTrack(arg))
        return;

    m_pBufferPool = arg.pBufferPool;
    m_IsInitialized = true;
}

void StreamSoundPlayer::ItemData::Set(const SetupArg& arg) {
    pitch = arg.pitch;
    mainSend = arg.mainSend / 127.0f - 1.0f;

    for (int i{0}; i < AuxBus_Count; ++i)
        fxSend[i] = arg.fxSend[i] / 127.0f;
}

bool StreamSoundPlayer::SetupTrack(const SetupArg& arg) {
    uint32_t bitMask{arg.allocTrackFlag};
    uint32_t trackIndex{0};

    while (bitMask != 0) {
        if (bitMask & 1) {
            if (trackIndex > StreamTrackCount - 1)
                break;
            m_Tracks[trackIndex].m_ActiveFlag = true;
        }
        bitMask >>= 1;
        ++trackIndex;
    }

    m_TrackCount = trackIndex <= 8 ? trackIndex : 8;

    if (m_TrackCount == 0) {
        Finalize();
        return false;
    }

    for (int i{0}; i < static_cast<int>(StreamTrackCount); ++i) {
        TrackDataInfo& data{m_StreamDataInfo.trackInfo[i]};
        data = arg.trackInfos.track[i];
    }

    return true;
}

void StreamSoundPlayer::Prepare(const PrepareArg& arg) {
    if (!m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().RegisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = true;
    }

    if (m_pLoader == nullptr) {
        m_PrepareArg = arg;
        m_IsSucceedPrepare = false;
        return;
    }

    if (!m_IsInitialized)
        return;

    if (!m_IsPreparedPrefetch)
        SetPrepareBaseArg(arg.baseArg);

    m_ReportLoadingDelayFlag = false;
    m_IsStoppedByLoadingDelay = false;
    m_IsSucceedPrepare = true;

    RequestLoadHeader(arg);
}

void StreamSoundPlayer::SetPrepareBaseArg(const PrepareBaseArg& baseArg) {
    m_DelayCount = baseArg.delayCount != 0 ? baseArg.delayCount : ToDelayCount(baseArg.delayTime);
    m_UseDelayCount = m_DelayCount > 0;
    m_StartOffsetType = baseArg.startOffsetType;
    m_StartOffset = baseArg.offset;
    m_UpdateType = baseArg.updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = baseArg.subMixIndex;
#endif

    m_pLoader->SetRegionCallback(baseArg.regionCallback, baseArg.regionCallbackArg);
    m_pLoader->SetStreamSoundPlayer(this);
    m_pLoader->SetStreamDataInfo(&m_StreamDataInfo);
    m_pLoader->SetFileType(static_cast<StreamFileType>(m_FileType));
    m_pLoader->SetDecodeMode(m_DecodeMode);

    SetActiveFlag(true);
}

void StreamSoundPlayer::RequestLoadHeader(const PrepareArg& arg) {
    m_pLoader->SetLoopParameter(m_LoopFlag, m_LoopStart, m_LoopEnd);
    m_pLoader->SetFilePath(arg.baseArg.filePath, FilePathMax);
    m_pLoader->SetFileStreamHookParam(arg.baseArg.fileStreamHookParam);
    m_pLoader->SetExternalData(arg.baseArg.pExternalData, arg.baseArg.externalDataSize);
    m_pLoader->SetCacheBuffer(arg.cacheBuffer, arg.cacheSize);

    bool isStreamOpenFailureHalt{SoundSystem::detail_IsStreamOpenFailureHaltEnabled()};
    m_pLoader->InitializeFileStream(isStreamOpenFailureHalt);

    m_pLoader->RequestLoadHeader();
}

void StreamSoundPlayer::PreparePrefetch(const PreparePrefetchArg& arg) {
    if (!m_IsInitialized)
        return;

    m_pStreamPrefetchFile = arg.strmPrefetchFile;

    StreamSoundPrefetchFileReader reader;
    reader.Initialize(m_pStreamPrefetchFile);

    if (!ReadPrefetchFile(reader))
        return;

    SetPrepareBaseArg(arg.baseArg);

    if (reader.IsIncludeRegionInfo()) {
        m_StreamDataInfo.isRegionIndexCheckEnabled = reader.IsRegionIndexCheckAvailable();

        if (m_pLoader == nullptr)
            return;

        if (!m_pLoader->GetRegionManager().InitializeRegion(&reader, &m_StreamDataInfo))
            return;

        if (!m_pLoader->GetRegionManager().IsInFirstRegion()) {
            SetActiveFlag(false);
            return;
        }
    }

    if (!ApplyStreamDataInfo(m_StreamDataInfo))
        return;

    if (!SetupPlayer())
        return;

    if (!AllocVoices()) {
        FreeStreamBuffers();
        return;
    }

    m_IsPreparedPrefetch = true;
    LoadPrefetchBlocks(reader);
}

bool StreamSoundPlayer::ReadPrefetchFile(StreamSoundPrefetchFileReader& reader) {
    StreamSoundPrefetchFileReader::PrefetchDataInfo prefetchInfo;
    if (!reader.ReadPrefetchDataInfo(&prefetchInfo, 0))
        return false;

    m_PrefetchDataInfo.startFrame = prefetchInfo.startFrame;
    m_PrefetchDataInfo.prefetchSize = prefetchInfo.prefetchSize;
    m_PrefetchDataInfo.dataAddress = prefetchInfo.dataAddress;

    StreamSoundFile::StreamSoundInfo info;
    reader.ReadStreamSoundInfo(&info);

    m_StreamDataInfo.channelCount = reader.GetChannelCount();
    m_StreamDataInfo.SetStreamSoundInfo(info, reader.IsCrc32CheckAvailable());

    if (reader.IsCrc32CheckAvailable()) {
        m_IsPrefetchRevisionCheckEnabled = true;
        m_PrefetchRevisionValue = info.crc32Value;
    } else {
        m_IsPrefetchRevisionCheckEnabled = false;
        m_PrefetchRevisionValue = 0;
    }

    m_ChannelCount = std::min<int32_t>(m_StreamDataInfo.channelCount, StreamChannelCount);
    return true;
}

bool StreamSoundPlayer::ApplyStreamDataInfo(const StreamDataInfoDetail& streamDataInfo) {
    if (!IsValidStartOffset(streamDataInfo)) {
        SetFinishFlag(true);
        Stop();
        return false;
    }

    ApplyTrackDataInfo(streamDataInfo);
    return true;
}

bool StreamSoundPlayer::SetupPlayer() {
    if (m_StreamDataInfo.blockSize > StreamSoundLoader::DataBlockSizeBase)
        return false;

    const size_t strmBufferSize{
        m_pBufferPool->GetBlockSize() /
        (m_StreamDataInfo.blockSize + StreamSoundLoader::DataBlockSizeMargin)};

    m_BufferBlockCount = strmBufferSize;

    if (m_BufferBlockCount < StreamSoundLoader::LoadBufferChannelCount)
        return false;

    if (m_BufferBlockCount > 32)
        m_BufferBlockCount = 32;

    m_LoadingBufferBlockIndex = 0;
    m_PlayingBufferBlockIndex = 0;
    m_LastPlayFinishBufferBlockIndex = 0;
    return true;
}

bool StreamSoundPlayer::AllocVoices() {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex) {
        StreamChannel& channel{m_Channels[channelIndex]};

        MultiVoice* voice{
            MultiVoiceManager::GetInstance().AllocVoice(1, 0xff, VoiceCallbackFunc, &channel)};

        if (voice == nullptr) {
            for (int i{0}; i < channelIndex; ++i) {
                StreamChannel& c{m_Channels[i]};
                if (c.m_pVoice != nullptr) {
                    c.m_pVoice->Free();
                    c.m_pVoice = nullptr;
                }
            }
            return false;
        }

        channel.m_pVoice = voice;
    }

    return true;
}

bool StreamSoundPlayer::LoadPrefetchBlocks(StreamSoundPrefetchFileReader& reader) {
    PrefetchIndexInfo indexInfo;
    indexInfo.Initialize(m_StreamDataInfo);

    position_t sampleBeginPosition{0};

    // Present in dwarf info, but maybe unused?
    [[maybe_unused]] size_t usedPrefetchMaxSize{0};

    for (int blockIndex{0}; blockIndex < m_BufferBlockCount; ++blockIndex) {
        PrefetchLoadDataParam loadDataParam;
        loadDataParam.Initialize();
        loadDataParam.prefetchBlockIndex = 0;
        loadDataParam.prefetchBlockBytes = 0;

        loadDataParam.sampleBegin = sampleBeginPosition;

        uint32_t blockOffsetFromLoopEnd{0};
        if (indexInfo.IsOverLastBlock(blockIndex))
            blockOffsetFromLoopEnd = indexInfo.GetBlockOffsetFromLoopEnd(blockIndex);

        if (indexInfo.IsLastBlock(blockIndex, blockOffsetFromLoopEnd)) {
            PreparePrefetchOnLastBlock(&loadDataParam, indexInfo);
        } else if (m_StreamDataInfo.loopFlag && indexInfo.IsOverLastBlock(blockIndex)) {
            if (blockOffsetFromLoopEnd != 1)
                PreparePrefetchOnLoopBlock(&loadDataParam, indexInfo, blockOffsetFromLoopEnd);
            else if (PreparePrefetchOnLoopStartBlock(&loadDataParam, indexInfo, reader))
                sampleBeginPosition = 0;
            else
                return false;

        } else {
            if (!PreparePrefetchOnNormalBlock(&loadDataParam, blockIndex, reader))
                return false;
        }

        sampleBeginPosition += loadDataParam.samples;
        loadDataParam.blockIndex = m_LoadingBufferBlockIndex;
        loadDataParam.sampleOffset = 0;
        LoadStreamData(true, loadDataParam, m_AssignNumber, true, loadDataParam.prefetchBlockIndex,
                       loadDataParam.prefetchBlockBytes);

        if (m_LoadingBufferBlockIndex + 1 < static_cast<uint32_t>(m_BufferBlockCount))
            ++m_LoadingBufferBlockIndex;
        else
            m_LoadingBufferBlockIndex = 0;

        if (loadDataParam.lastBlockFlag)
            break;
    }

    return true;
}

void StreamSoundPlayer::Start() {
    if (!m_UseDelayCount && !IsStarted())
        StartPlayer();
}

void StreamSoundPlayer::StartPlayer() {
    for (int trackIndex{0}; trackIndex < m_TrackCount; ++trackIndex) {
        StreamTrack& track{m_Tracks[trackIndex]};

        if (!track.m_ActiveFlag)
            continue;

        for (int ch{0}; ch < track.channelCount; ++ch) {
            StreamChannel* channel{track.m_pChannels[ch]};
            if (channel == nullptr)
                continue;

            MultiVoice* voice{channel->m_pVoice};
            if (voice == nullptr)
                continue;

            voice->SetSampleFormat(m_StreamDataInfo.sampleFormat);
            voice->SetSampleRate(m_StreamDataInfo.sampleRate);
            voice->SetUpdateType(m_UpdateType);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
            voice->SetOutputReceiver(GetOutputReceiver());
#endif
            voice->Start();
        }
    }

    UpdatePauseStatus();
    SetStartedFlag(true);
}

void StreamSoundPlayer::UpdatePauseStatus() {
    bool pauseStatus{(IsPause() | m_LoadWaitFlag) != 0};

    if (pauseStatus != m_PauseStatus) {
        for (int ch{0}; ch < m_ChannelCount; ++ch) {
            MultiVoice* voice{m_Channels[ch].m_pVoice};

            if (voice != nullptr)
                voice->Pause(pauseStatus);
        }

        m_PauseStatus = pauseStatus;
    }
}

bool StreamSoundPlayer::IsValidStartOffset(const StreamDataInfoDetail& streamDataInfo) {
    if (!streamDataInfo.loopFlag) {
        if (GetStartOffsetSamples(streamDataInfo) >= streamDataInfo.sampleCount)
            return false;
    }

    return true;
}

position_t StreamSoundPlayer::GetStartOffsetSamples(const StreamDataInfoDetail& streamDataInfo) {
    position_t startOffsetSamples{0};

    switch (m_StartOffsetType) {
    case StartOffsetType_Sample:
        startOffsetSamples = m_StartOffset;
        break;
    case StartOffsetType_Millisec:
        startOffsetSamples =
            (static_cast<uint64_t>(m_StartOffset) * streamDataInfo.sampleRate) / 1000;
        startOffsetSamples = fnd::Clamp<uint64_t>(startOffsetSamples, 0, 0xffffffff);
        break;
    }

    return startOffsetSamples;
}

void StreamSoundPlayer::PrefetchIndexInfo::Initialize(const StreamDataInfoDetail& streamDataInfo) {
    lastBlockIndex = streamDataInfo.GetLastBlockIndex();
    loopStartInBlock = streamDataInfo.GetLoopStartInBlock();
    loopStartBlockIndex = streamDataInfo.GetLoopStartBlockIndex(loopStartInBlock);
    loopBlockCount = (lastBlockIndex + 1) - loopStartBlockIndex;
}

void StreamSoundPlayer::PreparePrefetchOnLastBlock(PrefetchLoadDataParam* param,
                                                   const PrefetchIndexInfo& indexInfo) {
    param->samples = m_StreamDataInfo.lastBlockSampleCount;
    param->prefetchBlockBytes = m_StreamDataInfo.lastBlockSize;
    param->prefetchBlockIndex = indexInfo.lastBlockIndex;

    if (m_StreamDataInfo.loopFlag) {
        ++param->loopCount;
        m_PrefetchOffset = m_StreamDataInfo.loopStart;
    } else {
        param->lastBlockFlag = true;
        m_PrefetchOffset = 0;
    }
}

bool StreamSoundPlayer::PreparePrefetchOnLoopStartBlock(PrefetchLoadDataParam* param,
                                                        const PrefetchIndexInfo& indexInfo,
                                                        StreamSoundPrefetchFileReader& reader) {
    param->samples = m_StreamDataInfo.blockSampleCount;
    param->prefetchBlockBytes = m_StreamDataInfo.blockSize;
    param->prefetchBlockIndex = indexInfo.loopStartBlockIndex;
    param->sampleBegin = indexInfo.loopStartInBlock;

    m_PrefetchOffset = m_StreamDataInfo.blockSampleCount * (indexInfo.loopStartBlockIndex + 1);

    if (m_StreamDataInfo.sampleFormat == SampleFormat_DspAdpcm) {
        if (!SetAdpcmLoopInfo(reader, m_StreamDataInfo, m_PrefetchAdpcmParam, param->adpcmContext))
            return false;

        param->adpcmContextEnable = true;
    }

    return true;
}

void StreamSoundPlayer::PreparePrefetchOnLoopBlock(PrefetchLoadDataParam* param,
                                                   const PrefetchIndexInfo& indexInfo,
                                                   uint32_t blockOffsetFromLoopEnd) {
    param->samples = m_StreamDataInfo.blockSampleCount;
    param->prefetchBlockBytes = m_StreamDataInfo.blockSize;
    param->prefetchBlockIndex = indexInfo.loopStartBlockIndex + blockOffsetFromLoopEnd - 1;

    m_PrefetchOffset += m_StreamDataInfo.blockSampleCount;
}

bool StreamSoundPlayer::PreparePrefetchOnNormalBlock(PrefetchLoadDataParam* param,
                                                     uint32_t blockIndex,
                                                     StreamSoundPrefetchFileReader& reader) {
    param->samples = m_StreamDataInfo.blockSampleCount;
    param->prefetchBlockBytes = m_StreamDataInfo.blockSize;
    param->prefetchBlockIndex = blockIndex;

    m_PrefetchOffset += m_StreamDataInfo.blockSampleCount;

    if (m_StreamDataInfo.sampleFormat == SampleFormat_DspAdpcm && param->prefetchBlockIndex == 0) {
        if (!SetAdpcmLoopInfo(reader, m_StreamDataInfo, m_PrefetchAdpcmParam, param->adpcmContext))
            return false;

        param->adpcmContextEnable = true;
    }

    return true;
}

}  // namespace nn::atk::detail::driver
