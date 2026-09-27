#include <nn/atk/atk_SoundArchiveFile.h>

#include <cstring>

namespace nn::atk::detail {
namespace {

const uint32_t DefaultStringId{0xffffffff};
const PanMode DefaultPanMode{PanMode_Dual};
const PanCurve DefaultPanCurve{PanCurve_Sqrt};
const SinglePlayType DefaultSinglePlayType{SinglePlayType_None};
const uint16_t DefaultSinglePlayEffectiveDuration{0xffff};
const uint8_t DefaultPlayerPriority{64};
const uint8_t DefaultChannelPriority{64};
const uint8_t DefaultActorPlayerId{0};
const uint8_t DefaultIsReleasePriorityFix{0};
const bool DefaultIsFrontBypass{false};
const uint32_t DefaultUserParam{0xffffffff};
const uint32_t DefaultSeqStartOffset{0};
const uint32_t DefaultWarcWaveCount{0};
const uint32_t DefaultPlayerHeapSize{0};

enum SoundInfoBitFlag {
    SoundInfoBitFlag_StringId = 0,
    SoundInfoBitFlag_PanParam = 1,
    SoundInfoBitFlag_PlayerParam = 2,
    SoundInfoBitFlag_SinglePlayParam = 3,

    SoundInfoBitFlag_OffsetTo3dParam = 8,
    SoundInfoBitFlag_OffsetToSendParam = 9,
    SoundInfoBitFlag_OffsetToModParam = 10,
    SoundInfoBitFlag_OffsetToRvlParam = 16,
    SoundInfoBitFlag_OffsetToCtrParam = 17,
    SoundInfoBitFlag_OffsetToCafeParam = 18,

    SoundInfoBitFlag_UserParam3 = 28,
    SoundInfoBitFlag_UserParam2 = 29,
    SoundInfoBitFlag_UserParam1 = 30,
    SoundInfoBitFlag_UserParam = 31,
};

const int UserParamIndex[4]{SoundInfoBitFlag_UserParam, SoundInfoBitFlag_UserParam1,
                            SoundInfoBitFlag_UserParam2, SoundInfoBitFlag_UserParam3};

enum WaveSoundInfoBitFlag {
    WaveSoundInfoBitFlag_Priority = 0,
};

enum SequenceSoundInfoBitFlag {
    SequenceSoundInfoBitFlag_StartOffset = 0,
    SequenceSoundInfoBitFlag_Priority = 1,
};

enum BankInfoBitFlag {
    BankInfoBitFlag_StringId = 0,
};

enum PlayerInfoBitFlag {
    PlayerInfoBitFlag_StringId = 0,
    PlayerInfoBitFlag_HeapSize = 1,
};

enum SoundGroupInfoBitFlag {
    SoundGroupInfoBitFlag_StringId = 0,
};

enum GroupInfoBitFlag {
    GroupInfoBitFlag_StringId = 0,
};

enum WaveArchiveInfoBitFlag {
    WaveArchiveInfoBitFlag_StringId = 0,
    WaveArchiveInfoBitFlag_WaveCount = 1,
};

}  // anonymous namespace

const Util::ReferenceWithSize* SoundArchiveFile::FileHeader::GetReferenceBy(uint16_t typeId) const {
    for (int i{0}; i < BlockCount; ++i) {
        if (toBlocks[i].typeId == typeId)
            return &toBlocks[i];
    }

    return nullptr;
}

uint32_t SoundArchiveFile::FileHeader::GetStringBlockSize() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_StringBlock)->size;
}

uint32_t SoundArchiveFile::FileHeader::GetInfoBlockSize() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_InfoBlock)->size;
}

uint32_t SoundArchiveFile::FileHeader::GetFileBlockSize() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_FileBlock)->size;
}

int SoundArchiveFile::FileHeader::GetStringBlockOffset() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_StringBlock)->offset;
}

int SoundArchiveFile::FileHeader::GetInfoBlockOffset() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_InfoBlock)->offset;
}

int SoundArchiveFile::FileHeader::GetFileBlockOffset() const {
    return GetReferenceBy(ElementType_SoundArchiveFile_FileBlock)->offset;
}

const void* SoundArchiveFile::StringBlockBody::GetSection(Sections section) const {
    if (section > Sections_Max)
        return nullptr;

    return util::ConstBytePtr(this, toSection[section].offset).Get();
}

const char* SoundArchiveFile::StringBlockBody::GetString(SoundArchive::StringId stringId) const {
    if (stringId == SoundArchive::InvalidId)
        return nullptr;

    const StringTable* table{GetStringTable()};
    if (table == nullptr)
        return nullptr;

    return table->GetString(static_cast<int>(stringId));
}

void SoundArchiveFile::StringBlockBody::DumpTree() const {
    // TODO: Empty only in release builds
}

uint32_t SoundArchiveFile::StringBlockBody::GetItemIdImpl(Sections section, const char* str) const {
    const PatriciaTree* tree{GetPatriciaTree(section)};
    const PatriciaTree::NodeData* nodeData{tree->GetNodeDataBy(str)};

    if (nodeData == nullptr)
        return SoundArchive::InvalidId;

    const char* nodeDataStr{GetString(nodeData->stringId)};
    if (std::strcmp(str, nodeDataStr) != 0)
        return SoundArchive::InvalidId;

    return nodeData->itemId;
}

const SoundArchiveFile::PatriciaTree::NodeData*
SoundArchiveFile::PatriciaTree::GetNodeDataBy(const char* str, size_t len) const {
    if (rootIdx >= nodeTable.count)
        return nullptr;

    const Node* node = &nodeTable.item[rootIdx];
    if (len == 0)
        len = std::strlen(str);

    while ((node->flags & Node::FlagLeaf) == 0) {
        const int pos = node->bit >> 3;
        const int bit = node->bit & 7;
        uint32_t nodeIdx;

        if (pos < static_cast<int>(len) && str[pos] & (1 << (7 - bit)))
            nodeIdx = node->rightIdx;
        else
            nodeIdx = node->leftIdx;

        node = &nodeTable.item[nodeIdx];
    }

    return &node->nodeData;
}

const SoundArchiveFile::SoundInfo*
SoundArchiveFile::InfoBlockBody::GetSoundInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_Sound)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetSoundInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<SoundInfo>();
}

const SoundArchiveFile::BankInfo*
SoundArchiveFile::InfoBlockBody::GetBankInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_Bank)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetBankInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<BankInfo>();
}

const SoundArchiveFile::PlayerInfo*
SoundArchiveFile::InfoBlockBody::GetPlayerInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_Player)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetPlayerInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<PlayerInfo>();
}

const SoundArchiveFile::SoundGroupInfo*
SoundArchiveFile::InfoBlockBody::GetSoundGroupInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_SoundGroup)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetSoundGroupInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<SoundGroupInfo>();
}

const SoundArchiveFile::GroupInfo*
SoundArchiveFile::InfoBlockBody::GetGroupInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_Group)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetGroupInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<GroupInfo>();
}

const SoundArchiveFile::WaveArchiveInfo*
SoundArchiveFile::InfoBlockBody::GetWaveArchiveInfo(SoundArchive::ItemId itemId) const {
    if (Util::GetItemType(itemId) != ItemType_WaveArchive)
        return nullptr;

    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetWaveArchiveInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<WaveArchiveInfo>();
}

const SoundArchiveFile::FileInfo*
SoundArchiveFile::InfoBlockBody::GetFileInfo(SoundArchive::FileId itemId) const {
    uint32_t index{Util::GetItemIndex(itemId)};
    const Util::ReferenceTable& table{GetFileInfoReferenceTable()};

    if (index >= table.count)
        return nullptr;

    return util::ConstBytePtr(table.GetReferedItem(index)).Get<FileInfo>();
}

uint32_t SoundArchiveFile::SoundInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

uint32_t SoundArchiveFile::BankInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, BankInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

uint32_t SoundArchiveFile::WaveArchiveInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

uint32_t SoundArchiveFile::SoundGroupInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

uint32_t SoundArchiveFile::GroupInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, GroupInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

uint32_t SoundArchiveFile::PlayerInfo::GetStringId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, PlayerInfoBitFlag_StringId)};

    if (!result)
        return DefaultStringId;

    return value;
}

const SoundArchiveFile::SoundArchivePlayerInfo*
SoundArchiveFile::InfoBlockBody::GetSoundArchivePlayerInfo() const {
    return util::ConstBytePtr(this, toSoundArchivePlayerInfo.offset).Get<SoundArchivePlayerInfo>();
}

SoundArchive::FileId SoundArchiveFile::InfoBlockBody::GetItemFileId(SoundArchive::ItemId id) const {
    SoundArchive::FileId fileId{SoundArchive::InvalidId};

    switch (Util::GetItemType(id)) {
    case ItemType_Sound: {
        const SoundInfo* info{GetSoundInfo(id)};
        if (info != nullptr)
            fileId = info->fileId;
        break;
    }
    case ItemType_Bank: {
        const BankInfo* info{GetBankInfo(id)};
        if (info != nullptr)
            fileId = info->fileId;
        break;
    }
    case ItemType_WaveArchive: {
        const WaveArchiveInfo* info{GetWaveArchiveInfo(id)};
        if (info != nullptr)
            fileId = info->fileId;
        break;
    }
    case ItemType_Group: {
        const GroupInfo* info{GetGroupInfo(id)};
        if (info != nullptr)
            fileId = info->fileId;
        break;
    }
    case ItemType_SoundGroup: {
        const SoundGroupInfo* info{GetSoundGroupInfo(id)};
        if (info != nullptr) {
            SoundArchive::ItemId soundId{info->startId};
            const SoundInfo* soundInfo{GetSoundInfo(soundId)};
            if (soundInfo != nullptr)
                fileId = soundInfo->fileId;
        }
        break;
    }
    case ItemType_Player:
        break;
    }

    return fileId;
}

SoundArchive::StringId
SoundArchiveFile::InfoBlockBody::GetItemStringId(SoundArchive::ItemId id) const {
    SoundArchive::StringId stringId{SoundArchive::InvalidId};

    switch (Util::GetItemType(id)) {
    case ItemType_Sound: {
        const SoundInfo* info{GetSoundInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }
    case ItemType_Bank: {
        const BankInfo* info{GetBankInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }
    case ItemType_WaveArchive: {
        const WaveArchiveInfo* info{GetWaveArchiveInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }
    case ItemType_Group: {
        const GroupInfo* info{GetGroupInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }
    case ItemType_SoundGroup: {
        const SoundGroupInfo* info{GetSoundGroupInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }
    case ItemType_Player:
        const PlayerInfo* info{GetPlayerInfo(id)};
        if (info != nullptr)
            stringId = info->GetStringId();
        break;
    }

    return stringId;
}

SoundArchive::FileId
SoundArchiveFile::InfoBlockBody::GetItemPrefetchFileId(SoundArchive::ItemId id) const {
    SoundArchive::FileId fileId{SoundArchive::InvalidId};

    const SoundInfo* info{GetSoundInfo(id)};

    if (info != nullptr && info->GetSoundType() == SoundArchive::SoundType_Stream) {
        const SoundArchiveFile::StreamSoundInfo& streamSoundInfo{info->GetStreamSoundInfo()};
        fileId = streamSoundInfo.prefetchFileId;
    }

    return fileId;
}

const Util::ReferenceTable& SoundArchiveFile::InfoBlockBody::GetSoundInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toSoundInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const Util::ReferenceTable& SoundArchiveFile::InfoBlockBody::GetBankInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toBankInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const Util::ReferenceTable& SoundArchiveFile::InfoBlockBody::GetPlayerInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toPlayerInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const Util::ReferenceTable&
SoundArchiveFile::InfoBlockBody::GetSoundGroupInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toSoundGroupInfoReferenceTable.offset)
                .Get<Util::ReferenceTable>();
}

const Util::ReferenceTable&
SoundArchiveFile::InfoBlockBody::GetWaveArchiveInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toWaveArchiveInfoReferenceTable.offset)
                .Get<Util::ReferenceTable>();
}

const Util::ReferenceTable& SoundArchiveFile::InfoBlockBody::GetGroupInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toGroupInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const Util::ReferenceTable& SoundArchiveFile::InfoBlockBody::GetFileInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toFileInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

SoundArchive::SoundType SoundArchiveFile::SoundInfo::GetSoundType() const {
    switch (toDetailSoundInfo.typeId) {
    case ElementType_SoundArchiveFile_SequenceSoundInfo:
        return SoundArchive::SoundType_Sequence;

    case ElementType_SoundArchiveFile_StreamSoundInfo:
        return SoundArchive::SoundType_Stream;

    case ElementType_SoundArchiveFile_WaveSoundInfo:
        return SoundArchive::SoundType_Wave;

    default:
        return SoundArchive::SoundType_Invalid;
    }
}

const SoundArchiveFile::StreamSoundInfo& SoundArchiveFile::SoundInfo::GetStreamSoundInfo() const {
    return *util::ConstBytePtr(this, toDetailSoundInfo.offset).Get<StreamSoundInfo>();
}

const SoundArchiveFile::WaveSoundInfo& SoundArchiveFile::SoundInfo::GetWaveSoundInfo() const {
    return *util::ConstBytePtr(this, toDetailSoundInfo.offset).Get<WaveSoundInfo>();
}

const SoundArchiveFile::AdvancedWaveSoundInfo&
SoundArchiveFile::SoundInfo::GetAdvancedWaveSoundInfo() const {
    return *util::ConstBytePtr(this, toDetailSoundInfo.offset).Get<AdvancedWaveSoundInfo>();
}

const SoundArchiveFile::SequenceSoundInfo&
SoundArchiveFile::SoundInfo::GetSequenceSoundInfo() const {
    return *util::ConstBytePtr(this, toDetailSoundInfo.offset).Get<SequenceSoundInfo>();
}

const SoundArchiveFile::Sound3DInfo* SoundArchiveFile::SoundInfo::GetSound3DInfo() const {
    uint32_t offset;
    bool result{optionParameter.GetValue(&offset, SoundInfoBitFlag_OffsetTo3dParam)};

    if (!result)
        return nullptr;

    return util::ConstBytePtr(this, offset).Get<Sound3DInfo>();
}

PanMode SoundArchiveFile::SoundInfo::GetPanMode() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_PanParam)};

    if (!result)
        return DefaultPanMode;

    return static_cast<PanMode>(Util::DivideBy8bit(value, 0));
}

PanCurve SoundArchiveFile::SoundInfo::GetPanCurve() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_PanParam)};

    if (!result)
        return DefaultPanCurve;

    return static_cast<PanCurve>(Util::DivideBy8bit(value, 1));
}

SinglePlayType SoundArchiveFile::SoundInfo::GetSinglePlayType() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_SinglePlayParam)};

    if (!result)
        return DefaultSinglePlayType;

    return static_cast<SinglePlayType>(Util::DivideBy8bit(value, 0));
}

uint16_t SoundArchiveFile::SoundInfo::GetSinglePlayEffectiveDuration() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_SinglePlayParam)};

    if (!result)
        return DefaultSinglePlayEffectiveDuration;

    return Util::DivideBy16bit(value, 1);
}

uint8_t SoundArchiveFile::SoundInfo::GetPlayerPriority() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_PlayerParam)};

    if (!result)
        return DefaultPlayerPriority;

    return Util::DivideBy8bit(value, 0);
}

uint8_t SoundArchiveFile::SoundInfo::GetActorPlayerId() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_PlayerParam)};

    if (!result)
        return DefaultActorPlayerId;

    return Util::DivideBy8bit(value, 1);
}

uint32_t SoundArchiveFile::SoundInfo::GetUserParam() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_UserParam)};

    if (!result)
        return DefaultUserParam;

    return value;
}

bool SoundArchiveFile::SoundInfo::ReadUserParam(uint32_t* pOutValue, int index) const {
    return optionParameter.GetValue(pOutValue, UserParamIndex[index]);
}

bool SoundArchiveFile::SoundInfo::IsFrontBypass() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SoundInfoBitFlag_OffsetToCtrParam)};

    if (!result)
        return DefaultIsFrontBypass;

    return value & 1;
}

const SoundArchiveFile::StreamTrackInfoTable*
SoundArchiveFile::StreamSoundInfo::GetTrackInfoTable() const {
    if (!toTrackInfoTable.IsValidTypeId(ElementType_Table_ReferenceTable))
        return nullptr;

    return util::ConstBytePtr(this, toTrackInfoTable.offset).Get<StreamTrackInfoTable>();
}

const SoundArchiveFile::StreamSoundExtension*
SoundArchiveFile::StreamSoundInfo::GetStreamSoundExtension() const {
    if (!toStreamSoundExtension.IsValidOffset() ||
        !toStreamSoundExtension.IsValidTypeId(
            ElementType_SoundArchiveFile_StreamSoundExtensionInfo))
        return nullptr;

    return util::ConstBytePtr(this, toStreamSoundExtension.offset).Get<StreamSoundExtension>();
}

const SoundArchiveFile::SendValue& SoundArchiveFile::StreamSoundInfo::GetSendValue() const {
    return *util::ConstBytePtr(this, toSendValue.offset).Get<SendValue>();
}

uint8_t SoundArchiveFile::WaveSoundInfo::GetChannelPriority() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlag_Priority)};

    if (!result)
        return DefaultChannelPriority;

    return Util::DivideBy8bit(value, 0);
}

uint8_t SoundArchiveFile::WaveSoundInfo::GetIsReleasePriorityFix() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlag_Priority)};

    if (!result)
        return DefaultIsReleasePriorityFix;

    return Util::DivideBy8bit(value, 1);
}

const Util::Table<uint32_t>& SoundArchiveFile::SequenceSoundInfo::GetBankIdTable() const {
    return *util::ConstBytePtr(this, toBankIdTable.offset).Get<Util::Table<uint32_t>>();
}

void SoundArchiveFile::SequenceSoundInfo::GetBankIds(uint32_t* bankIds) const {
    const Util::Table<uint32_t>& table{GetBankIdTable()};

    for (uint32_t i{0}; i < SoundArchive::SequenceBankMax; ++i)
        bankIds[i] = i < table.count ? table.item[i] : SoundArchive::InvalidId;
}

uint32_t SoundArchiveFile::SequenceSoundInfo::GetStartOffset() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SequenceSoundInfoBitFlag_StartOffset)};

    if (!result)
        return DefaultSeqStartOffset;

    return value;
}

uint8_t SoundArchiveFile::SequenceSoundInfo::GetChannelPriority() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SequenceSoundInfoBitFlag_Priority)};

    if (!result)
        return DefaultChannelPriority;

    return Util::DivideBy8bit(value, 0);
}

bool SoundArchiveFile::SequenceSoundInfo::IsReleasePriorityFix() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, SequenceSoundInfoBitFlag_Priority)};

    if (!result)
        return DefaultIsReleasePriorityFix == 0;

    return Util::DivideBy8bit(value, 1) != 0;
}

uint32_t SoundArchiveFile::PlayerInfo::GetPlayerHeapSize() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, PlayerInfoBitFlag_HeapSize)};

    if (!result)
        return DefaultPlayerHeapSize;

    return value;
}

uint32_t SoundArchiveFile::WaveArchiveInfo::GetWaveCount() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveArchiveInfoBitFlag_WaveCount)};

    if (!result)
        return DefaultWarcWaveCount;

    return value;
}

SoundArchiveFile::FileLocationType SoundArchiveFile::FileInfo::GetFileLocationType() const {
    switch (toFileLocation.typeId) {
    case ElementType_SoundArchiveFile_InternalFileInfo:
        return FileLocationType_Internal;

    case ElementType_SoundArchiveFile_ExternalFileInfo:
        return FileLocationType_External;

    case 0:
        return FileLocationType_None;

    default:
        return FileLocationType_None;
    }
}

const SoundArchiveFile::InternalFileInfo* SoundArchiveFile::FileInfo::GetInternalFileInfo() const {
    if (GetFileLocationType() != FileLocationType_Internal)
        return nullptr;

    return util::ConstBytePtr(this, toFileLocation.offset).Get<InternalFileInfo>();
}

const SoundArchiveFile::ExternalFileInfo* SoundArchiveFile::FileInfo::GetExternalFileInfo() const {
    if (GetFileLocationType() != FileLocationType_External)
        return nullptr;

    return util::ConstBytePtr(this, toFileLocation.offset).Get<ExternalFileInfo>();
}

}  // namespace nn::atk::detail
