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
GetStreamTrackInfo(uint32_t index, SoundArchiveFile::StreamTrackInfoTable& table) {}

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

}  // namespace nn::atk::detail
