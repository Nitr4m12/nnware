#include <nn/atk/atk_StreamSoundFile.h>

#include <nn/atk/atk_ElementType.h>

namespace nn::atk::detail {

bool StreamSoundFile::FileHeader::HasSeekBlock() const {
    return GetReferenceBy(ElementType_StreamSoundFile_SeekBlock) != nullptr;
}

bool StreamSoundFile::FileHeader::HasRegionBlock() const {
    return GetReferenceBy(ElementType_StreamSoundFile_RegionBlock) != nullptr;
}

uint32_t StreamSoundFile::FileHeader::GetInfoBlockSize() const {
    return GetReferenceBy(ElementType_StreamSoundFile_InfoBlock)->size;
}

uint32_t StreamSoundFile::FileHeader::GetSeekBlockSize() const {
    return GetReferenceBy(ElementType_StreamSoundFile_SeekBlock)->size;
}

uint32_t StreamSoundFile::FileHeader::GetDataBlockSize() const {
    return GetReferenceBy(ElementType_StreamSoundFile_DataBlock)->size;
}

uint32_t StreamSoundFile::FileHeader::GetRegionBlockSize() const {
    return GetReferenceBy(ElementType_StreamSoundFile_RegionBlock)->size;
}

uint32_t StreamSoundFile::FileHeader::GetInfoBlockOffset() const {
    return GetReferenceBy(ElementType_StreamSoundFile_InfoBlock)->offset;
}

uint32_t StreamSoundFile::FileHeader::GetSeekBlockOffset() const {
    return GetReferenceBy(ElementType_StreamSoundFile_SeekBlock)->offset;
}

uint32_t StreamSoundFile::FileHeader::GetDataBlockOffset() const {
    return GetReferenceBy(ElementType_StreamSoundFile_DataBlock)->offset;
}

uint32_t StreamSoundFile::FileHeader::GetRegionBlockOffset() const {
    return GetReferenceBy(ElementType_StreamSoundFile_RegionBlock)->offset;
}

const Util::ReferenceWithSize* StreamSoundFile::FileHeader::GetReferenceBy(uint16_t typeId) const {
    for (int i{0}; i < dataBlocks; ++i) {
        const Util::ReferenceWithSize* p{&toBlocks[i]};
        if (p->typeId == typeId)
            return p;
    }

    return nullptr;
}

const StreamSoundFile::StreamSoundInfo* StreamSoundFile::InfoBlockBody::GetStreamSoundInfo() const {
    if (toStreamSoundInfo.IsValidTypeId(ElementType_StreamSoundFile_StreamSoundInfo))
        return util::ConstBytePtr(this).Advance(toStreamSoundInfo.offset).Get<StreamSoundInfo>();

    return nullptr;
}

const StreamSoundFile::TrackInfoTable* StreamSoundFile::InfoBlockBody::GetTrackInfoTable() const {
    if (toTrackInfoTable.IsValidTypeId(ElementType_Table_ReferenceTable))
        return util::ConstBytePtr(this).Advance(toTrackInfoTable.offset).Get<TrackInfoTable>();

    return nullptr;
}

const StreamSoundFile::ChannelInfoTable*
StreamSoundFile::InfoBlockBody::GetChannelInfoTable() const {
    if (toChannelInfoTable.IsValidTypeId(ElementType_Table_ReferenceTable))
        return util::ConstBytePtr(this).Advance(toChannelInfoTable.offset).Get<ChannelInfoTable>();

    return nullptr;
}

const StreamSoundFile::TrackInfo*
StreamSoundFile::TrackInfoTable::GetTrackInfo(uint32_t index) const {
    return static_cast<const TrackInfo*>(
        table.GetReferedItem(index, ElementType_StreamSoundFile_TrackInfo));
}

uint32_t StreamSoundFile::ChannelInfoTable::GetChannelCount() const {
    return table.count;
}

const StreamSoundFile::ChannelInfo*
StreamSoundFile::ChannelInfoTable::GetChannelInfo(uint32_t index) const {
    return static_cast<const ChannelInfo*>(
        table.GetReferedItem(index, ElementType_StreamSoundFile_ChannelInfo));
}

const StreamSoundFile::DspAdpcmChannelInfo*
StreamSoundFile::ChannelInfo::GetDspAdpcmChannelInfo() const {
    if (toDetailChannelInfo.IsValidTypeId(ElementType_Codec_DspAdpcmInfo))
        return util::ConstBytePtr(this)
            .Advance(toDetailChannelInfo.offset)
            .Get<DspAdpcmChannelInfo>();

    return nullptr;
}

}  // namespace nn::atk::detail
