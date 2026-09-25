#include <nn/atk/atk_StreamSoundLoader.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_SoundArchiveFilesHook.h>
#include <nn/atk/atk_TaskManager.h>
#include <nn/atk/atk_WaveFileReader.h>
#include <nn/atk/fnd/io/atkfnd_FileStreamImpl.h>
#include "nn/atk/fnd/io/atkfnd_Stream.h"

namespace {

using StreamDataDecoderManagerList = nn::util::IntrusiveList<
    nn::atk::detail::IStreamDataDecoderManager,
    nn::util::IntrusiveListMemberNodeTraits<nn::atk::detail::IStreamDataDecoderManager,
                                            &nn::atk::detail::IStreamDataDecoderManager::m_Link,
                                            nn::atk::detail::IStreamDataDecoderManager>>;

StreamDataDecoderManagerList g_StreamDataDecoderManagerList;

const uint8_t DefaultLpfFreq{64};
const uint8_t DefaultBiquadType{0};
const uint8_t DefaultBiquadValue{0};

const int LoopDecodeStartOffset{1};

}  // anonymous namespace

namespace nn::atk::detail {

void StreamDataInfoDetail::SetStreamSoundInfo(const StreamSoundFile::StreamSoundInfo& info,
                                              bool isCrc32CheckEnabled) {
    sampleFormat = WaveFileReader::GetSampleFormat(info.encodeMethod);
    sampleRate = static_cast<int>(info.sampleRate);
    loopFlag = info.isLoop;
    loopStart = info.loopStart;
    sampleCount = info.frameCount;
    originalLoopStart = info.originalLoopStart;
    originalLoopEnd = info.originalLoopEnd;
    blockSampleCount = info.oneBlockSamples;
    blockSize = info.oneBlockBytes;
    lastBlockSampleCount = info.lastBlockSamples;
    lastBlockSize = info.lastBlockPaddedBytes;

    revisionValue = info.crc32Value;
    isRevisionCheckEnabled = isCrc32CheckEnabled;

    regionCount = info.regionCount;
}

namespace driver {

StreamSoundLoader::StreamSoundLoader() {
    [[maybe_unused]] uint32_t taskCount = m_StreamDataLoadTaskPool.Create(
        m_StreamDataLoadTaskArea, DataBlockSizeBase - sizeof(StreamDataLoadTask) - 8);
    std::memset(m_FilePath, 0, sizeof(m_FilePath));
};

StreamSoundLoader::~StreamSoundLoader() {
    WaitFinalize();

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    if (g_pStreamDataDecoderManager != nullptr) {
        if (m_pStreamDataDecoder != nullptr) {
            g_pStreamDataDecoderManager->FreeImpl(m_pStreamDataDecoder);
            m_pStreamDataDecoder = nullptr;
        }
    }
#else
    if (m_pStreamDataDecoderManager != nullptr) {
        if (m_pStreamDataDecoder != nullptr) {
            m_pStreamDataDecoderManager->FreeImpl(m_pStreamDataDecoder);
            m_pStreamDataDecoder = nullptr;
        }
        m_pStreamDataDecoderManager = nullptr;
    }
#endif

    m_StreamDataLoadTaskPool.Destroy();
}

void StreamSoundLoader::WaitFinalize() {
    m_StreamHeaderLoadTask.Wait();
    m_StreamCloseTask.Wait();

    for (auto itr{m_StreamDataLoadTaskList.begin()}; itr != m_StreamDataLoadTaskList.end();) {
        auto curItr{itr++};
        StreamDataLoadTask* task{&*curItr};
        task->Wait();
        m_StreamDataLoadTaskList.erase(m_StreamDataLoadTaskList.iterator_to(*task));
        m_StreamDataLoadTaskPool.Free(task);
    }
}

void StreamSoundLoader::Initialize() {
    WaitFinalize();
    m_LoadingDataBlockIndex = 0;
    m_LastBlockIndex = 0xffffffff;
    m_LoopStartBlockIndex = 0;
    m_LoopStartFilePos = 0;
    m_LoopStartBlockSampleOffset = 0;
    m_LoopJumpFlag = false;
    m_LoadFinishFlag = false;
    m_RegionManager.Initialize();
    m_SampleFormat = SampleFormat_DspAdpcm;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    m_DecodeMode = DecodeMode_Invalid;
    m_pStreamDataDecoderManager = nullptr;
#endif
    m_pStreamDataDecoder = nullptr;
}

void StreamSoundLoader::Finalize() {
    CancelRequest();
    RequestClose();
}

void StreamSoundLoader::CancelRequest() {
    TaskManager::GetInstance().CancelTaskById(reinterpret_cast<uintptr_t>(this));
}

void StreamSoundLoader::RequestClose() {
    m_StreamCloseTask.Wait();
    m_StreamCloseTask.m_pLoader = this;
    TaskManager::GetInstance().AppendTask(&m_StreamCloseTask, TaskManager::TaskPriority_Middle);
}

void StreamSoundLoader::RegisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager) {
    g_StreamDataDecoderManagerList.push_back(*pManager);
}

void StreamSoundLoader::UnregisterStreamDataDecoderManager(IStreamDataDecoderManager* pManager) {
    g_StreamDataDecoderManagerList.erase(g_StreamDataDecoderManagerList.iterator_to(*pManager));
}

void* StreamSoundLoader::detail_SetFsAccessLog(fnd::FsAccessLog* pFsAccessLog) {
    if (m_pFileStream == nullptr)
        return nullptr;

    if (!m_pFileStream->CanSetFsAccessLog())
        return nullptr;

    return m_pFileStream->SetFsAccessLog(pFsAccessLog);
}

position_t StreamSoundLoader::detail_GetCurrentPosition() {
    if (m_pFileStream == nullptr)
        return 0;

    if (!m_pFileStream->IsCacheEnabled())
        return 0;

    return m_pFileStream->GetCurrentPosition();
}

position_t StreamSoundLoader::detail_GetCachePosition() {
    if (m_pFileStream == nullptr)
        return 0;

    if (!m_pFileStream->IsCacheEnabled())
        return 0;

    return m_pFileStream->GetCachePosition();
}

size_t StreamSoundLoader::detail_GetCachedLength() {
    if (m_pFileStream == nullptr)
        return 0;

    if (!m_pFileStream->IsCacheEnabled())
        return 0;

    return m_pFileStream->GetCachedLength();
}

void StreamSoundLoader::RequestLoadHeader() {
    m_StreamHeaderLoadTask.m_pLoader = this;
    m_StreamHeaderLoadTask.SetId(reinterpret_cast<uintptr_t>(this));
    TaskManager::GetInstance().AppendTask(&m_StreamHeaderLoadTask,
                                          TaskManager::TaskPriority_Middle);
}

void StreamSoundLoader::RequestLoadData(void** bufferAddress, uint32_t bufferBlockIndex,
                                        position_t startOffsetSamples,
                                        position_t prefetchOffsetSamples, int priority) {
    StreamDataLoadTask* task{m_StreamDataLoadTaskPool.Alloc()};

    if (task != nullptr)
        new (task) StreamDataLoadTask();

    task->m_pLoader = this;
    task->m_BufferBlockIndex = bufferBlockIndex;
    task->m_PrefetchOffsetSamples = prefetchOffsetSamples;
    task->m_StartOffsetSamples = startOffsetSamples;
    task->SetId(reinterpret_cast<uintptr_t>(this));

    for (int ch{0}; ch < m_ChannelCount; ++ch)
        task->m_BufferAddress[ch] = bufferAddress[ch];

    m_StreamDataLoadTaskList.push_back(*task);
    TaskManager::GetInstance().AppendTask(task, static_cast<TaskManager::TaskPriority>(priority));
}

void StreamSoundLoader::Update() {
    for (auto itr{m_StreamDataLoadTaskList.begin()}; itr != m_StreamDataLoadTaskList.end();) {
        auto curItr{itr++};
        StreamDataLoadTask* task{&*curItr};
        switch (task->GetStatus()) {
        case Task::Status_Done:
        case Task::Status_Cancel:
            task->Wait();
            m_StreamDataLoadTaskList.erase(m_StreamDataLoadTaskList.iterator_to(*task));
            m_StreamDataLoadTaskPool.Free(task);
            break;

        default:
            return;
        }
    }
}

void StreamSoundLoader::ForceFinish() {
    DriverCommand& cmdmgr{DriverCommand::GetInstanceForTaskThread()};
    DriverCommandStreamSoundForceFinish* command{
        cmdmgr.AllocCommand<DriverCommandStreamSoundForceFinish>(false)};
    command->id = DriverCommandId_StrmForceFinish;
    command->player = m_PlayerHandle;
    cmdmgr.PushCommand(command);
    cmdmgr.FlushCommand(true, false);
}

bool StreamSoundLoader::IsBusy() const {
    if (m_LoadFinishFlag)
        return false;

    return !m_StreamDataLoadTaskList.empty();
}

bool StreamSoundLoader::IsInUse() {
    Update();
    return !m_StreamDataLoadTaskList.empty();
}

fnd::FndResult StreamSoundLoader::Open() {
    if (m_pExternalData != nullptr) {
        m_pFileStream =
            new (m_FileStreamBuffer) MemoryFileStream(m_pExternalData, m_ExternalDataSize);
    } else {
        if (m_FileStreamHookParam.IsHookEnabled())
            m_pFileStream = m_FileStreamHookParam.pSoundArchiveFilesHook->OpenFile(
                m_FileStreamBuffer, sizeof(m_FileStreamBuffer), m_pCacheBuffer, m_CacheSize,
                m_FileStreamHookParam.itemLabel, SoundArchiveFilesHook::FileTypeStreamBinary);

        if (m_pFileStream == nullptr) {
            fnd::FileStream* stream{new (m_FileStreamBuffer) fnd::FileStreamImpl()};
            fnd::FndResult result{stream->Open(m_FilePath, fnd::FileStream::AccessMode_Read)};
            if (result.IsFailed())
                return result;

            if (stream->IsOpened() && IsStreamCacheEnabled())
                stream->EnableCache(m_pCacheBuffer, m_CacheSize);

            m_pFileStream = stream;
        }
    }

    m_FileLoader.Initialize(m_pFileStream);

    return fnd::FndResult{fnd::FndResultType_True};
}

void StreamSoundLoader::Close() {
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    if (g_pStreamDataDecoderManager != nullptr && m_pStreamDataDecoder != nullptr) {
        g_pStreamDataDecoderManager->FreeImpl(m_pStreamDataDecoder);
        m_pStreamDataDecoder = nullptr;
    }
#else
    if (m_pStreamDataDecoderManager != nullptr && m_pStreamDataDecoder != nullptr) {
        m_pStreamDataDecoderManager->FreeImpl(m_pStreamDataDecoder);
        m_pStreamDataDecoder = nullptr;
    }
#endif

    if (m_pFileStream == nullptr)
        return;

    m_pFileStream->Close();
    m_pFileStream = nullptr;
    m_FileLoader.Finalize();
}

void StreamSoundLoader::LoadHeader() {
    DriverCommand& cmdmgr{DriverCommand::GetInstanceForTaskThread()};
    auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundLoadHeader>(false)};

    command->id = DriverCommandId_StrmLoadHeader;
    command->player = m_PlayerHandle;
    command->assignNumber = m_AssignNumber;

    bool result{false};
    switch (m_FileType) {
    case StreamFileType_Bfstm:
        result = LoadHeader1(command);
        break;
    case StreamFileType_Opus:
        result = LoadHeaderForOpus(command, m_FileType, m_DecodeMode);
        break;
    }

    command->result = result;

    cmdmgr.PushCommand(command);
    cmdmgr.FlushCommand(true, false);
}

bool StreamSoundLoader::LoadHeader1(DriverCommandStreamSoundLoadHeader* command) {
    StreamSoundFileReader reader;

    if (!m_FileLoader.LoadFileHeader(&reader, g_LoadBuffer, sizeof(g_LoadBuffer)))
        return false;

    StreamSoundFile::StreamSoundInfo info;
    if (!reader.ReadStreamSoundInfo(&info))
        return false;

    uint32_t channelCount{reader.GetChannelCount()};

    m_ChannelCount = channelCount;
    m_DataInfo->channelCount = channelCount;
    m_DataInfo->SetStreamSoundInfo(info, reader.IsCrc32CheckAvailable());

    if (reader.IsTrackInfoAvailable() && !ReadTrackInfoFromStreamSoundFile(reader))
        return false;

    m_SampleFormat = m_DataInfo->sampleFormat;

    switch (m_SampleFormat) {
    case SampleFormat_DspAdpcm:
        if (!SetAdpcmInfo(reader, channelCount, command->adpcmParam))
            return false;
        break;
    default:
        for (uint32_t ch{0}; ch < channelCount; ++ch)
            command->adpcmParam[ch] = nullptr;
        break;
    }

    m_DataStartFilePos = reader.GetSampleDataOffset();
    m_LastBlockIndex = m_DataInfo->GetLastBlockIndex();
    m_LoopStartBlockIndex = m_DataInfo->GetLoopStartBlockIndex(0);

    m_LoopStartFilePos =
        m_DataStartFilePos + m_DataInfo->blockSize * m_ChannelCount * m_LoopStartBlockIndex;
    m_LoopStartBlockSampleOffset = 0;

    m_DataInfo->isRegionIndexCheckEnabled = reader.IsRegionIndexCheckAvailable();
    if (!m_RegionManager.InitializeRegion(&m_FileLoader, m_DataInfo))
        return false;

    UpdateLoadingDataBlockIndex();
    return true;
}

bool StreamSoundLoader::LoadHeaderForOpus(DriverCommandStreamSoundLoadHeader* command,
                                          StreamFileType type, DecodeMode decodeMode) {
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    if (g_pStreamDataDecoderManager == nullptr)
        return false;
#else
    DecodeMode actualDecodeMode{decodeMode == DecodeMode_Default ? DecodeMode_Cpu : decodeMode};
    m_pStreamDataDecoderManager = SelectStreamDataDecoderManager(type, actualDecodeMode);
    if (m_pStreamDataDecoderManager == nullptr)
        return false;
#endif

    if (m_pStreamDataDecoder == nullptr) {
        m_pStreamDataDecoder = m_pStreamDataDecoderManager->AllocImpl();
        if (m_pStreamDataDecoder == nullptr)
            return false;
    }

    IStreamDataDecoder::DataInfo info;
    bool result{m_pStreamDataDecoder->ReadDataInfo(&info, m_pFileStream)};
    if (!result)
        return false;

    uint32_t channelCount = info.channelCount;
    m_ChannelCount = channelCount;
    m_DataInfo->channelCount = channelCount;
    SetStreamSoundInfoForOpus(info);
    m_LastBlockIndex = m_DataInfo->GetLastBlockIndex();
    if (m_DataInfo->loopFlag)
        m_DataInfo->lastBlockSampleCount =
            m_DataInfo->sampleCount - m_LastBlockIndex * m_DataInfo->blockSampleCount;
    else
        m_DataInfo->lastBlockSampleCount = m_DataInfo->blockSampleCount;

    for (uint32_t ch{0}; ch < channelCount; ++ch)
        command->adpcmParam[ch] = nullptr;

    uint32_t loopStartBlockIndex{m_DataInfo->GetLoopStartBlockIndex(0)};
    position_t loopStartBlockSampleOffset =
        m_DataInfo->loopStart - loopStartBlockIndex * m_DataInfo->blockSampleCount;

    if (m_DataInfo->loopFlag) {
        size_t loopStartToBlockEnd{m_DataInfo->blockSampleCount - loopStartBlockSampleOffset};
        if (loopStartBlockSampleOffset >= LoopDecodeStartOffset) {
            m_DataInfo->loopStart += loopStartToBlockEnd;
            m_DataInfo->lastBlockSampleCount += loopStartToBlockEnd;
        }

        if (m_DataInfo->lastBlockSampleCount < m_DataInfo->blockSampleCount) {
            m_DataInfo->loopStart += m_DataInfo->blockSampleCount;
            m_DataInfo->lastBlockSampleCount += m_DataInfo->blockSampleCount;
        }

        loopStartBlockIndex = m_DataInfo->GetLoopStartBlockIndex(0);
        loopStartBlockSampleOffset =
            m_DataInfo->loopStart - loopStartBlockIndex * m_DataInfo->blockSampleCount;
    }

    m_LoopStartBlockIndex = loopStartBlockIndex;
    m_LoopStartBlockSampleOffset = loopStartBlockSampleOffset;

    m_DataInfo->isRegionIndexCheckEnabled = false;
    return m_RegionManager.InitializeRegion(&m_FileLoader, m_DataInfo);
}

bool StreamSoundLoader::ReadTrackInfoFromStreamSoundFile(StreamSoundFileReader& reader) {
    uint32_t trackCount{reader.GetTrackCount()};
    trackCount = trackCount < StreamTrackCount ? trackCount : StreamTrackCount;

    for (uint32_t i{0}; i < trackCount; ++i) {
        StreamSoundFileReader::TrackInfo trackInfo;
        if (!reader.ReadStreamTrackInfo(&trackInfo, i))
            return false;

        m_DataInfo->trackInfo[i].volume = trackInfo.volume;
        m_DataInfo->trackInfo[i].pan = trackInfo.pan;
        m_DataInfo->trackInfo[i].channelCount = trackInfo.channelCount;

        for (int ch{0}; ch < trackInfo.channelCount; ++ch)
            m_DataInfo->trackInfo[i].channelIndex[ch] = trackInfo.globalChannelIndex[ch];

        m_DataInfo->trackInfo[i].span = 0;
        m_DataInfo->trackInfo[i].flags = 0;
        m_DataInfo->trackInfo[i].mainSend = 127;
        for (int j{0}; j < AuxBus_Count; ++j)
            m_DataInfo->trackInfo[i].fxSend[j] = 0;

        m_DataInfo->trackInfo[i].lpfFreq = DefaultLpfFreq;
        m_DataInfo->trackInfo[i].biquadType = DefaultBiquadType;
        m_DataInfo->trackInfo[i].biquadValue = DefaultBiquadValue;
    }

    return true;
}

bool StreamSoundLoader::SetAdpcmInfo(StreamSoundFileReader& reader, int channelCount,
                                     AdpcmParam** adpcmParam) {
    for (int ch{0}; ch < channelCount; ++ch) {
        DspAdpcmParam dspAdpcmParam;
        DspAdpcmLoopParam dspAdpcmLoopParam;
        if (!reader.ReadDspAdpcmChannelInfo(&dspAdpcmParam, &dspAdpcmLoopParam, ch))
            return false;

        AdpcmInfo& adpcmInfo{m_AdpcmInfo[ch]};
        for (int i{0}; i < 8; ++i) {
            for (int j{0}; j < 2; ++j)
                adpcmInfo.param.coefficients[(i * sizeof(uint16_t)) + j] = dspAdpcmParam.coef[i][j];
        }
        adpcmInfo.beginContext.audioAdpcmContext.predScale = dspAdpcmParam.predScale;
        adpcmInfo.beginContext.audioAdpcmContext.history[0] = dspAdpcmParam.yn1;
        adpcmInfo.beginContext.audioAdpcmContext.history[1] = dspAdpcmParam.yn2;

        adpcmInfo.loopContext.audioAdpcmContext.predScale = dspAdpcmLoopParam.loopPredScale;
        adpcmInfo.loopContext.audioAdpcmContext.history[0] = dspAdpcmLoopParam.loopYn1;
        adpcmInfo.loopContext.audioAdpcmContext.history[1] = dspAdpcmLoopParam.loopYn2;

        adpcmParam[ch] = &adpcmInfo.param;
    }

    return true;
}

void StreamSoundLoader::UpdateLoadingDataBlockIndex() {
    m_LoadingDataBlockIndex =
        m_RegionManager.GetCurrentRegion().current / m_DataInfo->blockSampleCount;

    position_t startFilePos{static_cast<position_t>(
        m_DataStartFilePos + m_DataInfo->blockSize * m_ChannelCount * m_LoadingDataBlockIndex)};
    m_pFileStream->Seek(startFilePos, fnd::FileStream::SeekOrigin_Begin);
}

IStreamDataDecoderManager*
StreamSoundLoader::SelectStreamDataDecoderManager(StreamFileType type, DecodeMode decodeMode) {
    if (!g_StreamDataDecoderManagerList.empty()) {
        for (auto itr{g_StreamDataDecoderManagerList.begin()};
             itr != g_StreamDataDecoderManagerList.end(); ++itr) {
            if (itr->GetStreamFileTypeImpl() == type && itr->GetDecodeModeImpl() == decodeMode)
                return &*itr;
        }
    }

    char decoderName[32];
    switch (decodeMode) {
    case DecodeMode_Cpu:
        util::SNPrintf(decoderName, sizeof(decoderName), "OpusDecoder");
        break;
    case DecodeMode_Accelerator:
        util::SNPrintf(decoderName, sizeof(decoderName), "HardwareOpusDecoder");
        break;
    default:
        util::SNPrintf(decoderName, sizeof(decoderName), "Decoder");
        break;
    }

    return nullptr;
}

void StreamSoundLoader::SetStreamSoundInfoForOpus(const IStreamDataDecoder::DataInfo& info) {
    m_DataInfo->sampleFormat = SampleFormat_PcmS16;
    m_DataInfo->sampleRate = info.sampleRate;
    m_DataInfo->loopFlag = m_LoopFlag;
    m_DataInfo->loopStart = m_LoopStart;
    m_DataInfo->sampleCount = m_LoopEnd;
    m_DataInfo->originalLoopStart = m_LoopStart;
    m_DataInfo->originalLoopEnd = m_LoopEnd;
    m_DataInfo->blockSampleCount = info.blockSampleCount;
    m_DataInfo->blockSize = info.blockSize;
    m_DataInfo->lastBlockSize = m_DataInfo->blockSize;
}

// NON_MATCHING: x5 shouldn't be set when calling LoadDataForOpus, but it currently is
void StreamSoundLoader::LoadData(void** bufferAddress, uint32_t bufferBlockIndex,
                                 size_t startOffsetSamples, size_t prefetchOffsetSamples,
                                 TaskProfileLogger& logger) {
    if (m_LoadFinishFlag)
        return;

    DriverCommand& cmdmgr{DriverCommand::GetInstanceForTaskThread()};
    auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundLoadData>(false)};

    command->id = DriverCommandId_StrmLoadData;
    command->assignNumber = m_AssignNumber;

    bool result{false};
    if (m_pFileStream != nullptr) {
        switch (m_FileType) {
        case StreamFileType_Bfstm:
            result = LoadData1(command, bufferAddress, bufferBlockIndex, startOffsetSamples,
                               prefetchOffsetSamples, logger);
            break;
        case StreamFileType_Opus:
            result = LoadDataForOpus(command, bufferAddress, bufferBlockIndex, startOffsetSamples,
                                     prefetchOffsetSamples, logger);
            break;
        }
    }

    command->result = result;
    command->player = m_PlayerHandle;
    cmdmgr.PushCommand(command);
    cmdmgr.FlushCommand(true, false);
}

// NON_MATCHING: mismatch is too big to pinpoint.
bool StreamSoundLoader::LoadData1(DriverCommandStreamSoundLoadData* command, void** bufferAddress,
                                  uint32_t bufferBlockIndex, size_t startOffsetSamples,
                                  size_t prefetchOffsetSamples, TaskProfileLogger& logger) {
    const os::Tick beginTick{os::GetSystemTick()};
    LoadDataParam& loadDataParam{command->loadDataParam};

    position_t startOffsetSamplesInFrame{0};
    bool updateAdpcmContext{true};
    int loopCount{0};

    if (prefetchOffsetSamples == 0 && startOffsetSamples == 0) {
        updateAdpcmContext = false;
    } else {
        if (!ApplyStartOffset(startOffsetSamples + prefetchOffsetSamples, &loopCount)) {
            loadDataParam.samples = 0;
            return true;
        }
        UpdateLoadingDataBlockIndex();
        updateAdpcmContext = m_SampleFormat == SampleFormat_DspAdpcm;
    }

    size_t totalBlockSamples{0};
    position_t destAddressOffset{0};
    position_t sampleBegin{0};

    bool firstBlock{prefetchOffsetSamples == 0};
    bool isFirstDataLoad{true};

    loadDataParam.isStartOffsetOfLastBlockApplied = false;

    while (totalBlockSamples < DataBlockSizeMarginSamples ||
           m_RegionManager.GetCurrentRegion().Rest() < DataBlockSizeMarginSamples) {
        BlockInfo blockInfo;
        CalculateBlockInfo(blockInfo);
        auto samples = blockInfo.samples;
        auto copyByte = blockInfo.copyByte;

        if (prefetchOffsetSamples == 0) {
            startOffsetSamplesInFrame = blockInfo.GetStartOffsetInFrame();
            if (updateAdpcmContext && !LoadAdpcmContextForStartOffset())
                return false;
        }

        bool result;
        if (IsStreamCacheEnabled()) {
            result = LoadOneBlockDataViaCache(bufferAddress, blockInfo, destAddressOffset,
                                              firstBlock, updateAdpcmContext);
        } else {
            result = LoadOneBlockData(bufferAddress, blockInfo, destAddressOffset, firstBlock,
                                      updateAdpcmContext);
        }

        if (!result)
            return false;

        totalBlockSamples += samples;
        destAddressOffset += copyByte;
        m_RegionManager.AddPosition(samples);
        ++m_LoadingDataBlockIndex;

        if (m_RegionManager.GetCurrentRegion().IsEnd()) {
            if (MoveNextRegion(&loopCount))
                UpdateLoadingDataBlockIndex();

            if (prefetchOffsetSamples == 0 && startOffsetSamples == 0) {
                // loadDataParam.isStartOffsetOfLastBlockApplied = false;
            } else {
                if (isFirstDataLoad && totalBlockSamples < DataBlockSizeMarginSamples)
                    loadDataParam.isStartOffsetOfLastBlockApplied = true;
            }
            break;
        }

        firstBlock = false;
        isFirstDataLoad = false;
    }

    sampleBegin = m_RegionManager.GetCurrentRegion().current;
    loadDataParam.adpcmContextEnable = false;
    if (m_SampleFormat == SampleFormat_DspAdpcm) {
        if (sampleBegin == 0) {
            for (int ch{0}; ch < m_ChannelCount; ++ch)
                loadDataParam.adpcmContext[ch].audioAdpcmContext =
                    m_AdpcmInfo[ch].beginContext.audioAdpcmContext;
            loadDataParam.adpcmContextEnable = true;

        } else if (m_DataInfo->loopFlag && sampleBegin == m_DataInfo->loopStart) {
            for (int ch{0}; ch < m_ChannelCount; ++ch)
                loadDataParam.adpcmContext[ch].audioAdpcmContext =
                    m_AdpcmInfo[ch].loopContext.audioAdpcmContext;
            loadDataParam.adpcmContextEnable = true;

        } else if (sampleBegin == m_RegionManager.GetStartOffsetFrame()) {
            for (int ch{0}; ch < m_ChannelCount; ++ch)
                loadDataParam.adpcmContext[ch].audioAdpcmContext =
                    m_RegionManager.GetAdpcmContextForStartOffset(ch).audioAdpcmContext;
            loadDataParam.adpcmContextEnable = true;
        }
    }

    loadDataParam.blockIndex = bufferBlockIndex;
    loadDataParam.samples = totalBlockSamples;
    loadDataParam.sampleBegin = sampleBegin;
    loadDataParam.sampleOffset = startOffsetSamplesInFrame;
    loadDataParam.sampleBytes = destAddressOffset;
    loadDataParam.loopCount = loopCount;
    loadDataParam.lastBlockFlag = m_LoadFinishFlag;

    if (logger.IsProfilingEnabled()) {
        const os::Tick endTick{os::GetSystemTick()};

        TaskProfile profile;
        profile.type = TaskProfile::TaskProfileType_LoadStreamBlock;

        IStreamDataDecoder::CacheProfile cacheProfile;

        if (IsStreamCacheEnabled()) {
            cacheProfile.cacheStartPosition = detail_GetCachePosition();
            cacheProfile.cachedLength = detail_GetCachedLength();
            cacheProfile.cacheCurrentPosition = detail_GetCurrentPosition();
            cacheProfile.player = m_PlayerHandle;
        }

        profile.loadStreamBlock.SetData(beginTick, endTick, cacheProfile);

        logger.Record(profile);
    }

    return true;
}

bool StreamSoundLoader::ApplyStartOffset(position_t startOffsetSamples, int* loopCount) {
    position_t startOffsetSamplesInRegion{startOffsetSamples};

    while (!m_RegionManager.GetCurrentRegion().IsIn(startOffsetSamplesInRegion)) {
        startOffsetSamplesInRegion +=
            m_RegionManager.GetCurrentRegion().current - m_RegionManager.GetCurrentRegion().end;

        if (!MoveNextRegion(loopCount))
            return false;
    }

    m_RegionManager.AddPosition(startOffsetSamplesInRegion);
    return true;
}

void StreamSoundLoader::CalculateBlockInfo(BlockInfo& blockInfo) {
    if (m_LoadingDataBlockIndex != m_LastBlockIndex) {
        blockInfo.size = m_DataInfo->blockSize;
        blockInfo.samples = m_DataInfo->blockSampleCount;
    } else {
        blockInfo.size = m_DataInfo->lastBlockSize;
        blockInfo.samples = m_DataInfo->lastBlockSampleCount;
    }

    blockInfo.startOffsetSamples =
        m_RegionManager.GetCurrentRegion().current -
        (m_RegionManager.GetCurrentRegion().current / m_DataInfo->blockSampleCount) *
            m_DataInfo->blockSampleCount;
    blockInfo.startOffsetSamplesAlign = blockInfo.startOffsetSamples;

    if (m_SampleFormat == SampleFormat_DspAdpcm) {
        blockInfo.startOffsetSamplesAlign = (blockInfo.startOffsetSamplesAlign / 14);
        blockInfo.startOffsetSamplesAlign *= 14L << 32;
        blockInfo.startOffsetSamplesAlign =
            static_cast<int64_t>(blockInfo.startOffsetSamplesAlign) >> 32;
    }

    blockInfo.startOffsetByte =
        Util::GetByteBySample(blockInfo.startOffsetSamplesAlign, m_SampleFormat);
    blockInfo.copyByte = blockInfo.size - blockInfo.startOffsetByte;
    blockInfo.samples -= blockInfo.startOffsetSamples;

    if (!m_RegionManager.GetCurrentRegion().IsInWithBorder(blockInfo.samples)) {
        blockInfo.samples = m_RegionManager.GetCurrentRegion().Rest();
        if (blockInfo.samples < DataBlockSizeMarginSamples)
            blockInfo.copyByte = DataBlockSizeMargin;
    }
}

bool StreamSoundLoader::LoadAdpcmContextForStartOffset() {
    position_t fpos{m_pFileStream->GetCurrentPosition()};

    uint16_t yn1[StreamChannelCount];
    uint16_t yn2[StreamChannelCount];

    if (!m_FileLoader.ReadSeekBlockData(yn1, yn2, m_LoadingDataBlockIndex, m_ChannelCount))
        return false;

    m_pFileStream->Seek(fpos, fnd::FileStream::SeekOrigin_Begin);

    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        m_RegionManager.GetAdpcmContextForStartOffset(ch).audioAdpcmContext.history[0] = yn1[ch];
        m_RegionManager.GetAdpcmContextForStartOffset(ch).audioAdpcmContext.history[1] = yn2[ch];
    }

    return true;
}

bool StreamSoundLoader::LoadOneBlockDataViaCache(void** bufferAddress, const BlockInfo& blockInfo,
                                                 position_t destAddressOffset, bool firstBlock,
                                                 bool updateAdpcmContext) {
    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        if (firstBlock && updateAdpcmContext) {
            if (m_PlayerHandle->IsFinalizing())
                return false;

            uint8_t* dest{util::BytePtr(bufferAddress[ch], destAddressOffset).Get<uint8_t>()};
            SkipStreamBuffer(blockInfo.startOffsetByte);
            LoadStreamBuffer(dest, blockInfo.copyByte);
        }
    }

    return true;
}

bool StreamSoundLoader::MoveNextRegion(int* loopCount) {
    if (m_RegionManager.TryMoveNextRegion(&m_FileLoader, m_DataInfo)) {
        *loopCount = *loopCount + 1;
        return true;
    }

    m_LoadFinishFlag = true;
    return false;
}

bool StreamSoundLoader::LoadStreamBuffer(uint8_t* buffer, const BlockInfo& blockInfo,
                                         uint32_t loadChannelCount) {
    size_t loadSize{blockInfo.size * loadChannelCount};
    return m_pFileStream->Read(buffer, loadSize, nullptr) == loadSize;
}

bool StreamSoundLoader::LoadStreamBuffer(uint8_t* buffer, size_t size) {
    return m_pFileStream->Read(buffer, size, nullptr) == size;
}

bool StreamSoundLoader::SkipStreamBuffer(size_t skipSize) {
    return !m_pFileStream->Seek(skipSize, fnd::Stream::SeekOrigin_Current).IsFailed();
}

}  // namespace driver
}  // namespace nn::atk::detail
