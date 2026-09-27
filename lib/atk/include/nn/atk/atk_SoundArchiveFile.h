#pragma once

#include <nn/atk/atk_ElementType.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_StreamSoundFile.h>

namespace nn::atk::detail {

class SoundArchiveParametersHook;

class SoundArchiveFile {
public:
    static const int BlockCount{3};

    struct FileHeader : BinaryFileHeader {
        Util::ReferenceWithSize toBlocks[BlockCount];

        uint32_t GetStringBlockSize() const;
        uint32_t GetInfoBlockSize() const;
        uint32_t GetFileBlockSize() const;

        int GetStringBlockOffset() const;
        int GetInfoBlockOffset() const;
        int GetFileBlockOffset() const;

    private:
        const Util::ReferenceWithSize* GetReferenceBy(uint16_t typeId) const;
    };
    static_assert(sizeof(FileHeader) == 0x38);

    struct StringTable;
    struct PatriciaTree;
    struct StringBlockBody {
        enum Sections {
            Sections_StringTable = 0,
            Sections_PatriciaTree = 1,
            Sections_Max = 1,
        };

        Util::Reference toSection[1];

        const char* GetString(SoundArchive::StringId stringId) const;

        uint32_t GetStringCount() const { return GetStringTable()->GetCount(); }

        uint32_t GetItemId(const char* str) const {
            return GetItemIdImpl(Sections_PatriciaTree, str);
        }

        void DumpTree() const;

    private:
        const void* GetSection(Sections section) const;

        const StringTable* GetStringTable() const {
            return util::ConstBytePtr(GetSection(Sections_StringTable)).Get<StringTable>();
        }

        const PatriciaTree* GetPatriciaTree(Sections section) const {
            if (section > Sections_Max)
                return nullptr;

            return util::ConstBytePtr(GetSection(section)).Get<PatriciaTree>();
        }

        uint32_t GetItemIdImpl(Sections section, const char* str) const;
    };

    struct PatriciaTree {
        struct NodeData {
            uint32_t stringId;
            uint32_t itemId;
        };
        static_assert(sizeof(NodeData) == 0x8);

        struct Node {
            static const uint16_t FlagLeaf{1 << 0};

            uint16_t flags;
            uint16_t bit;
            uint32_t leftIdx;
            uint32_t rightIdx;
            NodeData nodeData;
        };
        static_assert(sizeof(Node) == 0x14);

        uint32_t rootIdx;
        Util::Table<Node> nodeTable;

        const NodeData* GetNodeDataBy(const char* str, size_t len = 0) const;

        void* operator[]([[maybe_unused]] int idx) const {
            // TODO
            return nullptr;
        }

        void* operator[]([[maybe_unused]] uint32_t idx) const {
            // TODO
            return nullptr;
        }

        void* operator[]([[maybe_unused]] const char* str) const {
            // TODO
            return nullptr;
        }

        void* operator()([[maybe_unused]] const char* str, [[maybe_unused]] size_t len) const {
            // TODO
            return nullptr;
        }

        uint32_t GetDataCount() const;
        uint32_t GetCount() const;
    };

    struct StringBlock {
        BinaryBlockHeader header;
        StringBlockBody body;
    };

    struct StringTable {
        Util::ReferenceWithSizeTable table;

        const char* GetString(int stringId) const {
            return util::ConstBytePtr(this, table.item[stringId].offset).Get<char>();
        }

        uint32_t GetCount() const { return table.count; }
    };

    struct SoundInfo;
    struct BankInfo;
    struct PlayerInfo;
    struct SoundGroupInfo;
    struct GroupInfo;
    struct WaveArchiveInfo;
    struct FileInfo;
    struct SoundArchivePlayerInfo;
    struct InfoBlockBody {
        Util::Reference toSoundInfoReferenceTable;
        Util::Reference toSoundGroupInfoReferenceTable;
        Util::Reference toBankInfoReferenceTable;
        Util::Reference toWaveArchiveInfoReferenceTable;
        Util::Reference toGroupInfoReferenceTable;
        Util::Reference toPlayerInfoReferenceTable;
        Util::Reference toFileInfoReferenceTable;
        Util::Reference toSoundArchivePlayerInfo;

        uint32_t GetSoundCount() const { return GetSoundInfoReferenceTable().count; }
        uint32_t GetBankCount() const { return GetBankInfoReferenceTable().count; }
        uint32_t GetPlayerCount() const { return GetPlayerInfoReferenceTable().count; }
        uint32_t GetSoundGroupCount() const { return GetSoundGroupInfoReferenceTable().count; }
        uint32_t GetGroupCount() const { return GetGroupInfoReferenceTable().count; }
        uint32_t GetWaveArchiveCount() const { return GetWaveArchiveInfoReferenceTable().count; }
        uint32_t GetFileCount() const { return GetFileInfoReferenceTable().count; }

        const SoundInfo* GetSoundInfo(SoundArchive::ItemId itemId) const;
        const BankInfo* GetBankInfo(SoundArchive::ItemId itemId) const;
        const PlayerInfo* GetPlayerInfo(SoundArchive::ItemId itemId) const;
        const SoundGroupInfo* GetSoundGroupInfo(SoundArchive::ItemId itemId) const;
        const GroupInfo* GetGroupInfo(SoundArchive::ItemId itemId) const;
        const WaveArchiveInfo* GetWaveArchiveInfo(SoundArchive::ItemId itemId) const;
        const FileInfo* GetFileInfo(SoundArchive::FileId itemId) const;

        const SoundArchivePlayerInfo* GetSoundArchivePlayerInfo() const;

        SoundArchive::FileId GetItemFileId(SoundArchive::ItemId id) const;
        SoundArchive::StringId GetItemStringId(SoundArchive::ItemId id) const;
        SoundArchive::FileId GetItemPrefetchFileId(SoundArchive::ItemId id) const;

    private:
        const Util::ReferenceTable& GetSoundInfoReferenceTable() const;
        const Util::ReferenceTable& GetBankInfoReferenceTable() const;
        const Util::ReferenceTable& GetPlayerInfoReferenceTable() const;
        const Util::ReferenceTable& GetSoundGroupInfoReferenceTable() const;
        const Util::ReferenceTable& GetWaveArchiveInfoReferenceTable() const;
        const Util::ReferenceTable& GetGroupInfoReferenceTable() const;
        const Util::ReferenceTable& GetFileInfoReferenceTable() const;
    };
    static_assert(sizeof(InfoBlockBody) == 0x40);

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };
    static_assert(sizeof(InfoBlock) == 0x48);

    struct StreamSoundInfo;
    struct WaveSoundInfo;
    struct AdvancedWaveSoundInfo;
    struct SequenceSoundInfo;
    struct Sound3DInfo;
    struct SoundInfo {
        uint32_t fileId;
        uint32_t playerId;
        uint8_t volume;
        uint8_t remoteFilter;
        uint8_t padding[2];
        Util::Reference toDetailSoundInfo;
        Util::BitFlag optionParameter;

        SoundArchive::SoundType GetSoundType() const;
        const StreamSoundInfo& GetStreamSoundInfo() const;
        const WaveSoundInfo& GetWaveSoundInfo() const;
        const AdvancedWaveSoundInfo& GetAdvancedWaveSoundInfo() const;
        const SequenceSoundInfo& GetSequenceSoundInfo() const;
        const Sound3DInfo* GetSound3DInfo() const;

        uint32_t GetStringId() const;
        PanMode GetPanMode() const;
        PanCurve GetPanCurve() const;
        SinglePlayType GetSinglePlayType() const;
        uint16_t GetSinglePlayEffectiveDuration() const;
        uint8_t GetPlayerPriority() const;
        uint8_t GetActorPlayerId() const;
        uint32_t GetUserParam() const;

        bool ReadUserParam(uint32_t* pOutValue, int index) const;

        bool IsFrontBypass() const;
    };
    static_assert(sizeof(SoundInfo) == 0x18);

    struct StreamTrackInfo;
    struct StreamTrackInfoTable {
        Util::ReferenceTable table;

        const StreamTrackInfo* GetTrackInfo(uint32_t index) {
            return util::ConstBytePtr(table.GetReferedItem(
                                          ElementType_SoundArchiveFile_StreamSoundTrackInfo, index))
                .Get<StreamTrackInfo>();
        }

        uint32_t GetTrackCount() const { return table.count; }
    };

    struct SendValue {
        uint8_t mainSend;
        uint8_t fxSend[3];
    };
    static_assert(sizeof(SendValue) == 0x4);

    struct StreamTrackInfo {
        uint8_t volume;
        uint8_t pan;
        uint8_t span;
        uint8_t flags;
        Util::Reference toGlobalChannelIndexTable;
        Util::Reference toSendValue;
        uint8_t lpfFreq;
        uint8_t biquadType;
        uint8_t biquadValue;
        uint8_t padding[1];

        uint32_t GetTrackChannelCount() const { return GetGlobalChannelIndexTable().GetCount(); }

        uint8_t GetGlobalChannelIndex(uint32_t index) const {
            return GetGlobalChannelIndexTable().GetGlobalIndex(index);
        }

        const SendValue& GetSendValue() const {
            return *util::ConstBytePtr(this, toSendValue.offset).Get<SendValue>();
        }

        const StreamSoundFile::GlobalChannelIndexTable& GetGlobalChannelIndexTable() const {
            return *util::ConstBytePtr(this, toGlobalChannelIndexTable.offset)
                        .Get<StreamSoundFile::GlobalChannelIndexTable>();
        }
    };
    static_assert(sizeof(StreamTrackInfo) == 0x18);

    struct StreamSoundExtension {
        uint32_t streamTypeInfo;
        uint32_t loopStartFrame;
        uint32_t loopEndFrame;

        SoundArchive::StreamFileType GetStreamFileType() const {
            switch (Util::DivideBy8bit(streamTypeInfo, 0)) {
            case 1:
                return SoundArchive::StreamFileType_NwStreamBinary;
            case 3:
                return SoundArchive::StreamFileType_Opus;
            default:
                return SoundArchive::StreamFileType_Invalid;
            }
        }

        bool IsLoop() const { return Util::DivideBy8bit(streamTypeInfo, 1) != 0; }

        SoundArchive::DecodeMode GetDecodeMode() const {
            switch (Util::DivideBy8bit(streamTypeInfo, 2)) {
            case 1:
                return SoundArchive::DecodeMode_Cpu;
            case 2:
                return SoundArchive::DecodeMode_Accelerator;
            default:
                return SoundArchive::DecodeMode_Default;
            }
        }
    };
    static_assert(sizeof(StreamSoundExtension) == 0xc);

    struct StreamSoundInfo {
        uint16_t allocateTrackFlags;
        uint16_t allocateChannelCount;
        Util::Reference toTrackInfoTable;
        float pitch;
        Util::Reference toSendValue;
        Util::Reference toStreamSoundExtension;
        uint32_t prefetchFileId;

        const StreamTrackInfoTable* GetTrackInfoTable() const;

        float GetPitch() const { return pitch; }
        const SendValue& GetSendValue() const;
        const StreamSoundExtension* GetStreamSoundExtension() const;
    };
    static_assert(sizeof(StreamSoundInfo) == 0x24);

    struct WaveSoundInfo {
        uint32_t index;
        uint32_t allocateTrackCount;
        Util::BitFlag optionParameter;

        uint8_t GetChannelPriority() const;
        uint8_t GetIsReleasePriorityFix() const;
    };
    static_assert(sizeof(WaveSoundInfo) == 0xc);

    struct AdvancedWaveSoundInfo {
        uint32_t waveArchiveId;
    };
    static_assert(sizeof(AdvancedWaveSoundInfo) == 0x4);

    struct SequenceSoundInfo {
        Util::Reference toBankIdTable;
        uint32_t allocateTrackFlags;
        Util::BitFlag optionParameter;

        void GetBankIds(uint32_t* bankIds) const;
        uint32_t GetStartOffset() const;
        uint8_t GetChannelPriority() const;
        bool IsReleasePriorityFix() const;

        const Util::Table<uint32_t>& GetBankIdTable() const;
    };
    static_assert(sizeof(SequenceSoundInfo) == 0x10);

    struct Sound3DInfo {
        uint32_t flags;
        float decayRatio;
        uint8_t decayCurve;
        uint8_t dopplerFactor;
        uint8_t padding[2];
        Util::BitFlag optionParameter;
    };
    static_assert(sizeof(Sound3DInfo) == 0x10);

    struct BankInfo {
        uint32_t fileId;
        Util::Reference toWaveArchiveItemIdTable;
        Util::BitFlag optionParameter;

        uint32_t GetStringId() const;

        const Util::Table<SoundArchive::ItemId>* GetWaveArchiveItemIdTable() const {
            return util::ConstBytePtr(this, toWaveArchiveItemIdTable.offset)
                .Get<Util::Table<SoundArchive::ItemId>>();
        }
    };
    static_assert(sizeof(BankInfo) == 0x10);

    struct PlayerInfo {
        uint32_t playableSoundMax;
        Util::BitFlag optionParameter;

        uint32_t GetStringId() const;
        uint32_t GetPlayerHeapSize() const;
    };
    static_assert(sizeof(PlayerInfo) == 0x8);

    struct WaveSoundGroupInfo;
    struct SoundGroupInfo {
        uint32_t startId;
        uint32_t endId;
        Util::Reference toFileIdTable;
        Util::Reference toDetailSoundGroupInfo;
        Util::BitFlag optionParameter;

        uint32_t GetStringId() const;

        const Util::Table<SoundArchive::FileId>* GetFileIdTable() const {
            return util::ConstBytePtr(this, toFileIdTable.offset)
                .Get<Util::Table<SoundArchive::FileId>>();
        }

        const WaveSoundGroupInfo* GetWaveSoundGroupInfo() const {
            return util::ConstBytePtr(this, toDetailSoundGroupInfo.offset)
                .Get<WaveSoundGroupInfo>();
        }
    };
    static_assert(sizeof(SoundGroupInfo) == 0x1c);

    struct WaveSoundGroupInfo {
        Util::Reference toWaveArchiveItemIdTable;
        Util::BitFlag optionParameter;

        const Util::Table<SoundArchive::ItemId>* GetWaveArchiveItemIdTable() const {
            return util::ConstBytePtr(this, toWaveArchiveItemIdTable.offset)
                .Get<Util::Table<SoundArchive::ItemId>>();
        }
    };
    static_assert(sizeof(WaveSoundGroupInfo) == 0xc);

    struct GroupInfo {
        uint32_t fileId;
        Util::BitFlag optionParameter;

        uint32_t GetStringId() const;
    };
    static_assert(sizeof(GroupInfo) == 0x8);

    struct WaveArchiveInfo {
        uint32_t fileId;
        bool isLoadIndividual;
        uint8_t padding[3];
        Util::BitFlag optionParameter;

        uint32_t GetStringId() const;
        uint32_t GetWaveCount() const;
    };
    static_assert(sizeof(WaveArchiveInfo) == 0xc);

    enum FileLocationType {
        FileLocationType_Internal,
        FileLocationType_External,
        FileLocationType_None,
    };

    struct InternalFileInfo;
    struct ExternalFileInfo;
    struct FileInfo {
        Util::Reference toFileLocation;
        Util::BitFlag optionParameter;

        FileLocationType GetFileLocationType() const;
        const InternalFileInfo* GetInternalFileInfo() const;
        const ExternalFileInfo* GetExternalFileInfo() const;
    };
    static_assert(sizeof(FileInfo) == 0xc);

    struct InternalFileInfo {
        static const uint32_t InvalidOffset{0xffffffff};
        static const uint32_t InvalidSize{0xffffffff};

        Util::ReferenceWithSize toFileImageFromFileBlockBody;
        Util::Reference toAttachedGroupIdTable;

        uint32_t GetFileSize() const { return toFileImageFromFileBlockBody.size; }

        uint32_t GetOffsetFromFileBlockHead() const { return toFileImageFromFileBlockBody.offset; }

        const Util::Table<uint32_t>* GetAttachedGroupTable() const {
            return util::ConstBytePtr(this, toAttachedGroupIdTable.offset)
                .Get<Util::Table<uint32_t>>();
        }
    };
    static_assert(sizeof(InternalFileInfo) == 0x14);

    struct ExternalFileInfo {
        char filePath[1];
    };

    struct SoundArchivePlayerInfo {
        uint16_t sequenceSoundCount;
        uint16_t sequenceTrackCount;
        uint16_t streamSoundCount;
        uint16_t streamTrackCount;
        uint16_t streamChannelCount;
        uint16_t waveSoundCount;
        uint16_t waveTrackCount;
        uint8_t streamBufferTimes;
        uint8_t developFlags;
        uint32_t options;
    };
    static_assert(sizeof(SoundArchivePlayerInfo) == 0x14);

    struct FileBlock {};
};

}  // namespace nn::atk::detail
