#include <nn/atk/atk_SoundArchiveFileReader.h>

namespace nn::atk::detail {

namespace {

const uint32_t SupportedFileVersionSar{NN_MAKE_VER(1, 0, 0)};
const uint32_t CurrentFileVersionSar{NN_MAKE_VER(2, 5, 0)};

const uint32_t StrmsendSupportedVersionSar{NN_MAKE_VER(2, 1, 0)};
const uint32_t FilterSupportedVersionSar{NN_MAKE_VER(2, 1, 0)};
const uint32_t StrmprefetchSupportedVersionSar{NN_MAKE_VER(2, 2, 0)};

bool IsValidFileHeaderSar(const void* soundArchiveData) {
    const BinaryFileHeader& header{*static_cast<const BinaryFileHeader*>(soundArchiveData)};

    bool isSupportedVersion{
        header.signature == SoundArchiveFileReader::SignatureFile &&
            header.byteOrder == BinaryFileHeader::ValidByteOrderMark &&
            header.version >= SupportedFileVersionSar && header.version <= CurrentFileVersionSar,
    };

    return isSupportedVersion;
}

const SoundArchiveFile::StreamTrackInfo*
GetStreamTrackInfo(uint32_t index, const SoundArchiveFile::StreamTrackInfoTable& table) {
    return static_cast<const SoundArchiveFile::StreamTrackInfo*>(
        table.table.GetReferedItem(index, ElementType_SoundArchiveFile_StreamSoundTrackInfo));
}

}  // anonymous namespace

SoundArchiveFileReader::SoundArchiveFileReader() = default;

void SoundArchiveFileReader::Initialize(const void* soundArchiveData) {
    if (IsValidFileHeaderSar(soundArchiveData))
        m_Header = *static_cast<const SoundArchiveFile::FileHeader*>(soundArchiveData);
}

void SoundArchiveFileReader::Finalize() {
    m_pStringBlockBody = nullptr;
    m_pInfoBlockBody = nullptr;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_pFileBlock = nullptr;
#endif
}

bool SoundArchiveFileReader::IsStreamSendAvailable() const {
    return m_Header.version >= StrmsendSupportedVersionSar;
}

bool SoundArchiveFileReader::IsFilterSupportedVersion() const {
    return m_Header.version >= FilterSupportedVersionSar;
}

bool SoundArchiveFileReader::IsStreamPrefetchAvailable() const {
    return m_Header.version >= StrmprefetchSupportedVersionSar;
}

void SoundArchiveFileReader::SetStringBlock(const void* stringBlock) {
    m_pStringBlockBody = &static_cast<const SoundArchiveFile::StringBlock*>(stringBlock)->body;
}

void SoundArchiveFileReader::SetInfoBlock(const void* infoBlock) {
    m_pInfoBlockBody = &static_cast<const SoundArchiveFile::InfoBlock*>(infoBlock)->body;
}

uint32_t SoundArchiveFileReader::GetStringCount() const {
    return m_pStringBlockBody->GetStringCount();
}

const char* SoundArchiveFileReader::GetString(SoundArchive::StringId stringId) const {
    return m_pStringBlockBody->GetString(stringId);
}

void SoundArchiveFileReader::DumpTree() const {
    m_pStringBlockBody->DumpTree();
}

SoundArchive::ItemId SoundArchiveFileReader::GetItemId(const char* pStr) const {
    if (m_pStringBlockBody == nullptr)
        return SoundArchive::InvalidId;

    return m_pStringBlockBody->GetItemId(pStr);
}

const char* SoundArchiveFileReader::GetItemLabel(SoundArchive::ItemId id) const {
    if (m_pStringBlockBody == nullptr)
        return nullptr;

    SoundArchive::StringId stringId{m_pInfoBlockBody->GetItemStringId(id)};
    if (stringId == SoundArchive::InvalidId)
        return nullptr;

    return m_pStringBlockBody->GetString(stringId);
}

SoundArchive::FileId SoundArchiveFileReader::GetItemFileId(SoundArchive::ItemId id) const {
    return m_pInfoBlockBody->GetItemFileId(id);
}

SoundArchive::FileId SoundArchiveFileReader::GetItemPrefetchFileId(SoundArchive::ItemId id) const {
    return m_pInfoBlockBody->GetItemPrefetchFileId(id);
}

uint32_t SoundArchiveFileReader::GetSoundCount() const {
    return m_pInfoBlockBody->GetSoundCount();
}

uint32_t SoundArchiveFileReader::GetBankCount() const {
    return m_pInfoBlockBody->GetBankCount();
}

uint32_t SoundArchiveFileReader::GetPlayerCount() const {
    return m_pInfoBlockBody->GetPlayerCount();
}

uint32_t SoundArchiveFileReader::GetSoundGroupCount() const {
    return m_pInfoBlockBody->GetSoundGroupCount();
}

uint32_t SoundArchiveFileReader::GetGroupCount() const {
    return m_pInfoBlockBody->GetGroupCount();
}

uint32_t SoundArchiveFileReader::GetWaveArchiveCount() const {
    return m_pInfoBlockBody->GetWaveArchiveCount();
}

uint32_t SoundArchiveFileReader::GetFileCount() const {
    return m_pInfoBlockBody->GetFileCount();
}

bool SoundArchiveFileReader::ReadSoundInfo(SoundArchive::ItemId soundId,
                                           SoundArchive::SoundInfo* info) const {
    const SoundArchiveFile::SoundInfo* data{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (data == nullptr)
        return false;

    info->fileId = data->fileId;
    info->playerId = data->playerId;
    info->volume = data->volume;
    info->panMode = data->GetPanMode();
    info->panCurve = data->GetPanCurve();
    info->singlePlayType = data->GetSinglePlayType();
    info->singlePlayEffectiveDuration = data->GetSinglePlayEffectiveDuration();
    info->playerPriority = data->GetPlayerPriority();
    info->actorPlayerId = data->GetActorPlayerId();
    info->remoteFilter = data->remoteFilter;
    info->isFrontBypass = data->IsFrontBypass();

    return true;
}

bool SoundArchiveFileReader::ReadBankInfo(SoundArchive::ItemId bankId,
                                          SoundArchive::BankInfo* info) const {
    const SoundArchiveFile::BankInfo* data{m_pInfoBlockBody->GetBankInfo(bankId)};
    if (data == nullptr)
        return false;

    info->fileId = data->fileId;

    return true;
}

bool SoundArchiveFileReader::ReadPlayerInfo(SoundArchive::ItemId playerId,
                                            SoundArchive::PlayerInfo* info) const {
    const SoundArchiveFile::PlayerInfo* data{m_pInfoBlockBody->GetPlayerInfo(playerId)};
    if (data == nullptr)
        return false;

    info->playableSoundMax = data->playableSoundMax;
    info->playerHeapSize = data->GetPlayerHeapSize();

    return true;
}

bool SoundArchiveFileReader::ReadSoundGroupInfo(SoundArchive::ItemId soundGroupId,
                                                SoundArchive::SoundGroupInfo* info) const {
    const SoundArchiveFile::SoundGroupInfo* data{m_pInfoBlockBody->GetSoundGroupInfo(soundGroupId)};
    if (data == nullptr)
        return false;

    info->startId = data->startId;
    info->endId = data->endId;
    info->fileIdTable = data->GetFileIdTable();

    return true;
}

bool SoundArchiveFileReader::ReadGroupInfo(SoundArchive::ItemId groupId,
                                           SoundArchive::GroupInfo* info) const {
    {
        const SoundArchiveFile::GroupInfo* data{m_pInfoBlockBody->GetGroupInfo(groupId)};
        if (data == nullptr)
            return false;

        info->fileId = data->fileId;
    }

    {
        SoundArchive::FileInfo fileInfo{};
        bool result{ReadFileInfo(info->fileId, &fileInfo, 0)};
        if (!result)
            return false;

        info->groupFileSize = fileInfo.fileSize;
    }

    return true;
}

bool SoundArchiveFileReader::ReadWaveArchiveInfo(SoundArchive::ItemId warcId,
                                                 SoundArchive::WaveArchiveInfo* info) const {
    const SoundArchiveFile::WaveArchiveInfo* data{m_pInfoBlockBody->GetWaveArchiveInfo(warcId)};
    if (data == nullptr)
        return false;

    info->fileId = data->fileId;
    info->waveCount = data->GetWaveCount();
    info->isLoadIndividual = data->isLoadIndividual;

    return true;
}

bool SoundArchiveFileReader::ReadFileInfo(SoundArchive::FileId id, SoundArchive::FileInfo* info,
                                          [[maybe_unused]] int32_t index) const {
    const SoundArchiveFile::FileInfo* data{m_pInfoBlockBody->GetFileInfo(id)};
    if (data == nullptr)
        return false;

    switch (data->GetFileLocationType()) {
    case SoundArchiveFile::FileLocationType_Internal: {
        const SoundArchiveFile::InternalFileInfo* location{data->GetInternalFileInfo()};
        info->fileSize = location->GetFileSize();
        info->offsetFromFileBlockHead = location->GetOffsetFromFileBlockHead();
        return true;
    }
    case SoundArchiveFile::FileLocationType_External: {
        const SoundArchiveFile::ExternalFileInfo* location{data->GetExternalFileInfo()};
        info->externalFilePath = location->filePath;
        return true;
    }
    default:
        return false;
    }
}

bool SoundArchiveFileReader::ReadSoundArchivePlayerInfo(
    SoundArchive::SoundArchivePlayerInfo* info) const {
    const SoundArchiveFile::SoundArchivePlayerInfo* data{
        m_pInfoBlockBody->GetSoundArchivePlayerInfo(),
    };
    if (data == nullptr)
        return false;

    info->sequenceSoundCount = data->sequenceSoundCount;
    info->sequenceTrackCount = data->sequenceTrackCount;
    info->streamSoundCount = data->streamSoundCount;
    info->streamTrackCount = data->streamTrackCount;
    info->streamChannelCount = data->streamChannelCount;
    info->waveSoundCount = data->waveSoundCount;
    info->waveTrackCount = data->waveTrackCount;
    info->streamBufferTimes = IsStreamPrefetchAvailable() ? data->streamBufferTimes : 1;
    info->isAdvancedWaveSoundEnabled = data->developFlags & 1;

    return true;
}

bool SoundArchiveFileReader::ReadSound3DInfo(SoundArchive::ItemId soundId,
                                             SoundArchive::Sound3DInfo* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr)
        return false;

    const SoundArchiveFile::Sound3DInfo* sound3dInfo{soundInfo->GetSound3DInfo()};
    if (sound3dInfo == nullptr)
        return false;

    info->flags = sound3dInfo->flags;
    info->decayRatio = sound3dInfo->decayRatio;
    info->decayCurve = sound3dInfo->decayCurve;
    info->dopplerFactor = sound3dInfo->dopplerFactor;

    return true;
}

bool SoundArchiveFileReader::ReadSequenceSoundInfo(SoundArchive::ItemId soundId,
                                                   SoundArchive::SequenceSoundInfo* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr || soundInfo->GetSoundType() != SoundArchive::SoundType_Sequence)
        return false;

    const SoundArchiveFile::SequenceSoundInfo& seqSoundInfo{soundInfo->GetSequenceSoundInfo()};

    info->startOffset = seqSoundInfo.GetStartOffset();
    info->allocateTrackFlags = seqSoundInfo.allocateTrackFlags;
    info->channelPriority = seqSoundInfo.GetChannelPriority();
    info->isReleasePriorityFix = seqSoundInfo.IsReleasePriorityFix();
    seqSoundInfo.GetBankIds(info->bankIds);

    return true;
}

bool SoundArchiveFileReader::ReadStreamSoundInfo(SoundArchive::ItemId soundId,
                                                 SoundArchive::StreamSoundInfo* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr || soundInfo->GetSoundType() != SoundArchive::SoundType_Stream)
        return false;

    const SoundArchiveFile::StreamSoundInfo& strmSoundInfo{soundInfo->GetStreamSoundInfo()};

    info->allocateTrackFlags = strmSoundInfo.allocateTrackFlags;
    info->allocateChannelCount = strmSoundInfo.allocateChannelCount;

    const SoundArchiveFile::StreamSoundExtension* strmSoundExt{
        strmSoundInfo.GetStreamSoundExtension(),
    };

    info->streamFileType = strmSoundExt != nullptr ? strmSoundExt->GetStreamFileType() :
                                                     SoundArchive::StreamFileType_NwStreamBinary;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    info->decodeMode =
        strmSoundExt != nullptr ? strmSoundExt->GetDecodeMode() : SoundArchive::DecodeMode_Default;
#endif

    if (IsStreamSendAvailable()) {
        info->pitch = strmSoundInfo.pitch;
        const SoundArchiveFile::SendValue send{strmSoundInfo.GetSendValue()};
        info->mainSend = send.mainSend;
        for (uint32_t i{0}; i < AuxBus_Count; ++i)
            info->fxSend[i] = send.fxSend[i];
    } else {
        info->pitch = 1.0f;
        info->mainSend = 127;
        for (uint32_t i{0}; i < AuxBus_Count; ++i)
            info->fxSend[i] = 0;
    }

    info->prefetchFileId =
        IsStreamPrefetchAvailable() ? strmSoundInfo.prefetchFileId : SoundArchive::InvalidId;

    const SoundArchiveFile::StreamTrackInfoTable* table{strmSoundInfo.GetTrackInfoTable()};
    if (table == nullptr)
        return true;

    uint32_t tableCount{table->GetTrackCount()};
    for (uint32_t i{0}; i < StreamTrackCount && i < tableCount; ++i) {
        SoundArchive::StreamTrackInfo& tmp{info->trackInfo[i]};
        const SoundArchiveFile::StreamTrackInfo* pTrackInfo{GetStreamTrackInfo(i, *table)};

        tmp.volume = pTrackInfo->volume;
        tmp.pan = pTrackInfo->pan;
        tmp.surroundPan = pTrackInfo->span;
        tmp.flags = pTrackInfo->flags;
        tmp.channelCount = pTrackInfo->GetTrackChannelCount();

        if (IsStreamSendAvailable()) {
            const SoundArchiveFile::SendValue send{pTrackInfo->GetSendValue()};
            tmp.mainSend = send.mainSend;
            for (uint32_t j{0}; j < AuxBus_Count; ++j)
                tmp.fxSend[j] = send.fxSend[j];
        } else {
            tmp.mainSend = 127;
            for (uint32_t j{0}; j < AuxBus_Count; ++j)
                tmp.fxSend[j] = 0;
        }

        if (IsFilterSupportedVersion()) {
            tmp.lowPassFilterFrequency = pTrackInfo->lpfFreq;
            tmp.biquadType = pTrackInfo->biquadType;
            tmp.biquadValue = pTrackInfo->biquadValue;
        } else {
            tmp.lowPassFilterFrequency = 64;
            tmp.biquadType = BiquadFilterType_None;
            tmp.biquadValue = 0;
        }

        uint32_t count{tmp.channelCount};
        if (tmp.channelCount >= 2)
            count = 2;

        for (uint32_t j{0}; j < count; ++j)
            tmp.globalChannelIndex[j] = pTrackInfo->GetGlobalChannelIndex(j);
    }

    return true;
}

bool SoundArchiveFileReader::ReadStreamSoundInfo2(SoundArchive::ItemId soundId,
                                                  SoundArchive::StreamSoundInfo2* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr || soundInfo->GetSoundType() != SoundArchive::SoundType_Stream)
        return false;

    const SoundArchiveFile::StreamSoundInfo& strmSoundInfo{soundInfo->GetStreamSoundInfo()};
    const SoundArchiveFile::StreamSoundExtension* strmSoundExt{
        strmSoundInfo.GetStreamSoundExtension(),
    };

    if (strmSoundExt == nullptr)
        return false;

    info->isLoop = strmSoundExt->IsLoop();
    info->loopStartFrame = strmSoundExt->loopStartFrame;
    info->loopEndFrame = strmSoundExt->loopEndFrame;
    return true;
}

bool SoundArchiveFileReader::ReadWaveSoundInfo(SoundArchive::ItemId soundId,
                                               SoundArchive::WaveSoundInfo* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr || soundInfo->GetSoundType() != SoundArchive::SoundType_Wave)
        return false;

    const SoundArchiveFile::WaveSoundInfo& waveSoundInfo{soundInfo->GetWaveSoundInfo()};

    info->index = waveSoundInfo.index;
    info->allocateTrackCount = waveSoundInfo.allocateTrackCount;
    info->channelPriority = waveSoundInfo.GetChannelPriority();
    info->isReleasePriorityFix = waveSoundInfo.GetIsReleasePriorityFix();

    return true;
}

bool SoundArchiveFileReader::ReadAdvancedWaveSoundInfo(
    SoundArchive::ItemId soundId, SoundArchive::AdvancedWaveSoundInfo* info) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr || soundInfo->GetSoundType() != SoundArchive::SoundType_Wave)
        return false;

    const SoundArchiveFile::AdvancedWaveSoundInfo& advancedWaveSoundInfo{
        soundInfo->GetAdvancedWaveSoundInfo(),
    };

    info->waveArchiveId = advancedWaveSoundInfo.waveArchiveId;

    return true;
}

const Util::Table<uint32_t>*
SoundArchiveFileReader::GetWaveArchiveIdTable(SoundArchive::ItemId id) const {
    switch (Util::GetItemType(id)) {
    case ItemType_Sound: {
        const SoundArchiveFile::SoundInfo* pInfo{m_pInfoBlockBody->GetSoundInfo(id)};
        if (pInfo->GetSoundType() != SoundArchive::SoundType_Wave)
            return nullptr;

        for (uint32_t i{0}; i < m_pInfoBlockBody->GetSoundGroupCount(); ++i) {
            const SoundArchiveFile::SoundGroupInfo* pGroupInfo;
            pGroupInfo =
                m_pInfoBlockBody->GetSoundGroupInfo(Util::GetMaskedItemId(i, ItemType_SoundGroup));

            if (pGroupInfo->startId <= id && id <= pGroupInfo->endId) {
                const SoundArchiveFile::WaveSoundGroupInfo* pWaveSoundGroupInfo{
                    pGroupInfo->GetWaveSoundGroupInfo(),
                };
                return pWaveSoundGroupInfo->GetWaveArchiveItemIdTable();
            }
        }

        return nullptr;
    }
    case ItemType_SoundGroup: {
        const SoundArchiveFile::SoundGroupInfo* pInfo{m_pInfoBlockBody->GetSoundGroupInfo(id)};

        const SoundArchiveFile::WaveSoundGroupInfo* pWaveSoundGroupInfo{
            pInfo->GetWaveSoundGroupInfo(),
        };
        return pWaveSoundGroupInfo->GetWaveArchiveItemIdTable();
    }
    case ItemType_Bank: {
        const SoundArchiveFile::BankInfo* pInfo{m_pInfoBlockBody->GetBankInfo(id)};

        return pInfo->GetWaveArchiveItemIdTable();
    }
    default:
        return nullptr;
    }
}

SoundArchive::SoundType SoundArchiveFileReader::GetSoundType(SoundArchive::ItemId soundId) const {
    const SoundArchiveFile::SoundInfo* soundInfo{m_pInfoBlockBody->GetSoundInfo(soundId)};
    if (soundInfo == nullptr)
        return SoundArchive::SoundType_Invalid;

    return soundInfo->GetSoundType();
}

}  // namespace nn::atk::detail
