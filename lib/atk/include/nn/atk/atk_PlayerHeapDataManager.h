#pragma once

#include <nn/atk/atk_DisposeCallback.h>
#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundArchiveLoader.h>

namespace nn::atk::detail {

class PlayerHeapDataManager : public driver::DisposeCallback, public SoundArchiveLoader {
public:
    static const int FileAddressCount{9};

    PlayerHeapDataManager();
    ~PlayerHeapDataManager() override;

    void Initialize(const SoundArchive* arc);
    void Finalize();

    const void* SetFileAddress(SoundArchive::FileId fileId, const void* address);
    const void* GetFileAddress(SoundArchive::FileId fileId) const;

protected:
    void InvalidateData(const void* start, const void* end) override;

    const void* SetFileAddressToTable(SoundArchive::FileId fileId, const void* address) override;
    const void* GetFileAddressFromTable(SoundArchive::FileId fileId) const override;
    const void* GetFileAddressImpl(SoundArchive::FileId fileId) const override;

private:
    struct FileAddress {
        SoundArchive::FileId fileId;
        const void* address;
    };
    static_assert(sizeof(FileAddress) == 0x10);

    FileAddress m_FileAddress[FileAddressCount];
    bool m_IsInitialized{false};
    bool m_IsFinalized{true};
};
static_assert(sizeof(PlayerHeapDataManager) == 0x2c8);

}  // namespace nn::atk::detail
