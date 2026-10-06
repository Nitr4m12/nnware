#pragma once

#include <nn/atk/atk_Config.h>
#include <nn/atk/atk_Util.h>
#include <nn/atk/fnd/io/atkfnd_FileStream.h>

namespace nn::atk {

namespace detail {

class SoundArchiveFileReader;
class SoundArchiveParametersHook;
class SoundArchiveFilesHook;

namespace driver {

class StreamSoundLoader;

}

}  // namespace detail

class SoundArchive {
public:
    using ItemId = uint32_t;

    static const ItemId InvalidId{0xffffffff};

    static const int UserParamIndexMax{3};
    static const uint32_t ResultInvalidSoundId{0};
    static const uint32_t InvalidUserParam{0xffffffff};

    enum SoundType {
        SoundType_Invalid,
        SoundType_Sequence,
        SoundType_Stream,
        SoundType_Wave,
        SoundType_AdvancedWave,
    };

    enum StreamFileType {
        StreamFileType_Invalid = 0,
        StreamFileType_NwStreamBinary = 1,
        StreamFileType_Opus = 3,
    };

    enum DecodeMode {
        DecodeMode_Default,
        DecodeMode_Cpu,
        DecodeMode_Accelerator,
    };

    using FileId = ItemId;
    using StringId = ItemId;

    struct SoundInfo {
        FileId fileId;
        ItemId playerId;
        uint8_t actorPlayerId;
        uint8_t playerPriority;
        uint8_t volume;
        uint8_t remoteFilter;
        PanMode panMode;
        PanCurve panCurve;
        SinglePlayType singlePlayType;
        uint16_t singlePlayEffectiveDuration;
        bool isFrontBypass;
    };
    static_assert(sizeof(SoundInfo) == 0x1c);

    struct Sound3DInfo {
        uint32_t flags;
        float decayRatio;
        uint8_t decayCurve;
        uint8_t dopplerFactor;
    };
    static_assert(sizeof(Sound3DInfo) == 0xc);

    static const uint32_t SequenceBankMax{4};
    struct SequenceSoundInfo {
        uint32_t startOffset{0};
        uint32_t bankIds[SequenceBankMax];
        uint32_t allocateTrackFlags{0};
        uint8_t channelPriority{0};
        bool isReleasePriorityFix{false};

        SequenceSoundInfo() {
            for (int i{0}; i < static_cast<int>(SequenceBankMax); ++i)
                bankIds[i] = InvalidId;
        }
    };
    static_assert(sizeof(SequenceSoundInfo) == 0x1c);

    static const uint32_t StreamTrackCount{8};
    struct StreamTrackInfo {
        uint8_t volume;
        uint8_t pan;
        uint8_t surroundPan;
        uint8_t flags;
        uint8_t mainSend;
        uint8_t fxSend[3];
        uint8_t lowPassFilterFrequency;
        uint8_t biquadType;
        uint8_t biquadValue;
        uint8_t channelCount;
        int8_t globalChannelIndex[2];

        StreamTrackInfo() = default;
    };
    static_assert(sizeof(StreamTrackInfo) == 0xe);

    struct StreamSoundInfo {
        uint16_t allocateTrackFlags;
        uint16_t allocateChannelCount;
        float pitch;
        uint8_t mainSend;
        uint8_t fxSend[3];
        StreamTrackInfo trackInfo[StreamTrackCount];
        StreamFileType streamFileType;
        DecodeMode decodeMode;
        FileId prefetchFileId;
        void* streamBufferPool;

        StreamSoundInfo() = default;
        void Setup();
    };
    static_assert(sizeof(StreamSoundInfo) == 0x90);

    struct StreamSoundInfo2 {
        bool isLoop;
        uint32_t loopStartFrame;
        uint32_t loopEndFrame;

        StreamSoundInfo2() = default;
    };
    static_assert(sizeof(StreamSoundInfo2) == 0xc);

    struct WaveSoundInfo {
        uint32_t index;
        uint32_t allocateTrackCount{0};
        uint8_t channelPriority{0};
        bool isReleasePriorityFix{false};

        WaveSoundInfo() = default;
    };
    static_assert(sizeof(WaveSoundInfo) == 0xc);

    struct AdvancedWaveSoundInfo {
        uint32_t waveArchiveId;
    };
    static_assert(sizeof(AdvancedWaveSoundInfo) == 0x4);

    struct BankInfo {
        FileId fileId{InvalidId};

        BankInfo() = default;
    };
    static_assert(sizeof(BankInfo) == 0x4);

    struct WaveArchiveInfo {
        uint32_t fileId{SoundArchive::InvalidId};
        uint32_t waveCount;
        bool isLoadIndividual{false};
        uint8_t padding[3];

        WaveArchiveInfo() = default;
    };
    static_assert(sizeof(WaveArchiveInfo) == 0xc);

    struct PlayerInfo {
        int playableSoundMax;
        uint32_t playerHeapSize;

        PlayerInfo() = default;
    };
    static_assert(sizeof(PlayerInfo) == 0x8);

    struct SoundGroupInfo {
        ItemId startId{InvalidId};
        ItemId endId{InvalidId};
        detail::Util::Table<FileId>* fileIdTable{};

        SoundGroupInfo() = default;
    };
    static_assert(sizeof(SoundGroupInfo) == 0x10);

    struct GroupInfo {
        FileId fileId{InvalidId};
        uint32_t groupFileSize{0};

        GroupInfo() = default;
    };
    static_assert(sizeof(GroupInfo) == 0x8);

    struct SoundArchivePlayerInfo {
        int32_t sequenceSoundCount;
        int32_t sequenceTrackCount;
        int32_t streamSoundCount;
        int32_t streamTrackCount;
        int32_t streamChannelCount;
        int32_t waveSoundCount;
        int32_t waveTrackCount;
        int32_t streamBufferTimes;
        bool isAdvancedWaveSoundEnabled;
    };
    static_assert(sizeof(SoundArchivePlayerInfo) == 0x24);

    struct FileInfo {
        static const uint32_t InvalidOffset{0xffffffff};
        static const uint32_t InvalidSize{0xffffffff};

        uint32_t fileSize{InvalidSize};
        uint32_t offsetFromFileBlockHead{InvalidOffset};
        const char* externalFilePath{};

        FileInfo() = default;
    };
    static_assert(sizeof(FileInfo) == 0x10);

protected:
    SoundArchive();

public:
    virtual ~SoundArchive();

    bool IsAvailable() const;

    uint32_t GetSoundCount() const;
    uint32_t GetGroupCount() const;
    uint32_t GetPlayerCount() const;
    uint32_t GetSoundGroupCount() const;
    uint32_t GetBankCount() const;
    uint32_t GetWaveArchiveCount() const;
    uint32_t detail_GetFileCount() const;

    const char* GetItemLabel(ItemId id) const;
    ItemId GetItemId(const char* pStr) const;
    FileId GetItemFileId(ItemId id) const;
    FileId GetItemPrefetchFileId(ItemId id) const;

    static ItemId GetSoundIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_Sound);
    }

    static ItemId GetSoundGroupIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_SoundGroup);
    }

    static ItemId GetBankIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_Bank);
    }

    static ItemId GetPlayerIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_Player);
    }

    static ItemId GetWaveArchiveIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_WaveArchive);
    }

    static ItemId GetGroupIdFromIndex(uint32_t index) {
        return detail::Util::GetMaskedItemId(index, detail::ItemType_Group);
    }

    uint32_t GetSoundUserParam(ItemId soundId) const;
    bool ReadSoundUserParam(uint32_t* pOutValue, ItemId soundId, int index) const;

    SoundType GetSoundType(ItemId soundId) const;

    bool ReadSoundInfo(SoundInfo* pOutValue, ItemId soundId) const;
    bool ReadSequenceSoundInfo(SequenceSoundInfo* pOutValue, ItemId soundId) const;
    bool ReadBankInfo(BankInfo* pOutValue, ItemId bankId) const;
    bool ReadPlayerInfo(PlayerInfo* pOutValue, ItemId playerId) const;
    bool ReadSoundArchivePlayerInfo(SoundArchivePlayerInfo* pOutValue) const;
    bool ReadStreamSoundInfo(StreamSoundInfo* pOutValue, ItemId soundId) const;

    bool detail_ReadStreamSoundInfo2(ItemId soundId, StreamSoundInfo2* info) const;
    bool detail_ReadWaveSoundInfo(ItemId soundId, WaveSoundInfo* info) const;
    bool detail_ReadAdvancedWaveSoundInfo(ItemId soundId, AdvancedWaveSoundInfo* info) const;

    bool ReadSound3DInfo(Sound3DInfo* pOutValue, ItemId soundId) const;
    bool ReadWaveArchiveInfo(ItemId warcId, WaveArchiveInfo* info) const;

    bool detail_ReadSoundGroupInfo(ItemId soundGroupId, SoundGroupInfo* info) const;

    bool ReadGroupInfo(GroupInfo* pOutValue, ItemId groupId) const;

    bool detail_ReadFileInfo(FileId fileId, FileInfo* info) const;

    const detail::Util::Table<uint32_t>* detail_GetWaveArchiveIdTable(ItemId id) const;
    virtual const void* detail_GetFileAddress(FileId fileId) const = 0;
    virtual size_t detail_GetRequiredStreamBufferSize() const = 0;

    detail::fnd::FileStream* detail_OpenFileStream(FileId fileId, void* buffer, size_t size,
                                                   void* cacheBuffer, size_t cacheSize) const;
    const detail::Util::Table<uint32_t>* detail_GetAttachedGroupTable(FileId fileId) const;

    detail::SoundArchiveParametersHook* detail_GetParametersHook() const {
        return m_pParametersHook;
    }

    void detail_SetParametersHook(detail::SoundArchiveParametersHook* parametersHook) {
        m_pParametersHook = parametersHook;
    }

    void SetExternalFileRoot(const char* extFileRoot);

    bool ReadStreamSoundFilePath(char* outFilePathBuffer, size_t filePathBufferSize,
                                 ItemId soundId) const;

    virtual void FileAccessBegin() const;
    virtual void FileAccessEnd() const;

    const char* detail_GetExternalFileFullPath(const char* externalFilePath, char* pathBuffer,
                                               size_t bufSize) const;

    virtual bool IsAddon() const;

protected:
    void Initialize(detail::SoundArchiveFileReader* fileReader);
    void Finalize();

    virtual detail::fnd::FileStream* OpenStream(void* buffer, size_t size, position_t begin,
                                                size_t length) const = 0;
    virtual detail::fnd::FileStream* OpenExtStream(void* buffer, size_t size,
                                                   const char* extFilePath, void* cacheBuffer,
                                                   size_t cacheSize) const = 0;

    detail::fnd::FileStream* OpenExtStreamImpl(void* buffer, size_t size,
                                               const char* externalFilePath, void* cacheBuffer,
                                               size_t cacheSize) const;

    static const int32_t FilePathMax{639};

private:
    friend detail::driver::StreamSoundLoader;

    detail::SoundArchiveFileReader* m_pFileReader{};
    detail::SoundArchiveParametersHook* m_pParametersHook{};
    char m_ExtFileRoot[FilePathMax];
    uint32_t m_FileBlockOffset;
};
static_assert(sizeof(SoundArchive) == 0x2a0);

class AddonSoundArchive : public SoundArchive {};

}  // namespace nn::atk
