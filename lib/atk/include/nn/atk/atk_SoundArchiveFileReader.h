#pragma once

#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundArchiveFile.h>

namespace nn::atk::detail {

class SoundArchiveFileReader {
public:
    constexpr static int32_t SignatureFile = 0x52415346;  // FSAR
    constexpr static int32_t InvalidOffset = -1;
    constexpr static int32_t InvalidSize = -1;

    SoundArchiveFileReader();

    void Initialize(const void* soundArchiveData);
    void Finalize();

    bool IsStreamSendAvailable() const;
    bool IsFilterSupportedVersion() const;
    bool IsStreamPrefetchAvailable() const;

    void SetStringBlock(const void* stringBlock);
    void SetInfoBlock(const void* infoBlock);

    int32_t GetStringCount() const;
    char* GetString(uint32_t) const;

    void DumpTree() const;

    SoundArchive::ItemId GetItemId(const char* pStr);
    const char* GetItemLabel(SoundArchive::ItemId id) const;

    SoundArchive::FileId GetItemFileId(SoundArchive::ItemId id) const;
    SoundArchive::FileId GetItemPrefetchFileId(SoundArchive::ItemId id) const;

    uint32_t GetSoundCount() const;
    uint32_t GetBankCount() const;
    uint32_t GetPlayerCount() const;
    uint32_t GetSoundGroupCount() const;
    uint32_t GetGroupCount() const;
    uint32_t GetWaveArchiveCount() const;
    uint32_t GetFileCount() const;

    bool ReadSoundInfo(SoundArchive::ItemId soundId, SoundArchive::SoundInfo* info) const;
    bool ReadBankInfo(SoundArchive::ItemId bankId, SoundArchive::BankInfo* info) const;
    bool ReadPlayerInfo(SoundArchive::ItemId playerId, SoundArchive::PlayerInfo* info) const;
    bool ReadSoundGroupInfo(SoundArchive::ItemId soundGroupId,
                            SoundArchive::SoundGroupInfo* info) const;
    bool ReadGroupInfo(SoundArchive::ItemId groupId, SoundArchive::GroupInfo* info) const;
    bool ReadFileInfo(SoundArchive::FileId id, SoundArchive::FileInfo* info, int32_t index) const;
    bool ReadWaveArchiveInfo(SoundArchive::ItemId warcId,
                             SoundArchive::WaveArchiveInfo* info) const;
    bool ReadSoundArchivePlayerInfo(SoundArchive::SoundArchivePlayerInfo* info) const;
    bool ReadSound3DInfo(SoundArchive::ItemId soundId, SoundArchive::Sound3DInfo* info) const;
    bool ReadSequenceSoundInfo(SoundArchive::ItemId soundId,
                               SoundArchive::SequenceSoundInfo* info) const;
    bool ReadStreamSoundInfo(SoundArchive::ItemId soundId,
                             SoundArchive::StreamSoundInfo* info) const;
    bool ReadStreamSoundInfo2(SoundArchive::ItemId soundId,
                              SoundArchive::StreamSoundInfo2* info) const;
    bool ReadWaveSoundInfo(SoundArchive::ItemId soundId, SoundArchive::WaveSoundInfo* info) const;
    bool ReadAdvancedWaveSoundInfo(SoundArchive::ItemId soundId,
                                   SoundArchive::AdvancedWaveSoundInfo* info) const;

    Util::Table<uint32_t>* GetWaveArchiveIdTable(SoundArchive::ItemId id) const;
    SoundArchive::SoundType GetSoundType(SoundArchive::ItemId soundId) const;
    uint32_t GetSoundUserParam(uint32_t) const;

    bool ReadSoundUserParam(uint32_t*, uint32_t, int32_t) const;
    const detail::Util::Table<uint32_t>* GetAttachedGroupTable(uint32_t) const;

    int GetInfoBlockOffset() const { return m_Header.GetInfoBlockOffset(); }
    int GetFileBlockOffset() const { return m_Header.GetFileBlockOffset(); }
    int GetStringBlockOffset() const { return m_Header.GetStringBlockOffset(); }

    uint32_t GetInfoBlockSize() const { return m_Header.GetInfoBlockSize(); }
    uint32_t GetFileBlockSize() const { return m_Header.GetFileBlockSize(); }
    uint32_t GetStringBlockSize() const { return m_Header.GetStringBlockSize(); }

private:
    friend SoundArchive;

    SoundArchiveFile::FileHeader m_Header;
    SoundArchiveFile::StringBlockBody* m_pStringBlockBody;
    SoundArchiveFile::InfoBlockBody* m_pInfoBlockBody;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    SoundArchiveFile::FileBlock* m_pFileBlock;
#endif
};

}  // namespace nn::atk::detail
