#pragma once

#include <nn/atk/atk_GroupFile.h>

namespace nn::atk::detail {

struct GroupItemLocationInfo {
    uint32_t fileId;
    const void* address;
};
static_assert(sizeof(GroupItemLocationInfo) == 0x10);

class GroupFileReader {
public:
    static const uint32_t SignatureFile{0x50524746};  // FGRP

    explicit GroupFileReader(const void* groupFile);

    uint32_t GetGroupItemCount() const { return m_pInfoBlockBody->GetGroupItemInfoCount(); }

    bool ReadGroupItemLocationInfo(GroupItemLocationInfo* out, uint32_t index) const;

    uint32_t GetGroupItemExCount() const;

    bool ReadGroupItemInfoEx(GroupFile::GroupItemInfoEx* out, uint32_t index) const;

private:
    const GroupFile::InfoBlockBody* m_pInfoBlockBody{};
    const GroupFile::FileBlockBody* m_pFileBlockBody{};
    const GroupFile::InfoExBlockBody* m_pInfoExBlockBody{};
};
static_assert(sizeof(GroupFileReader) == 0x18);

}  // namespace nn::atk::detail
