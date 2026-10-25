#pragma once

#include <nn/atk/atk_SoundArchive.h>
#include <nn/atk/atk_SoundArchiveFileReader.h>

namespace nn::atk {

class MemorySoundArchive : public SoundArchive {
public:
    MemorySoundArchive();
    ~MemorySoundArchive() override;

    bool Initialize(const void* buffer);
    void Finalize();

    const void* detail_GetFileAddress(FileId fileId) const override;

    size_t detail_GetRequiredStreamBufferSize() const override;

    detail::fnd::FileStream* OpenStream(void* buffer, size_t size, position_t begin,
                                                size_t length) const override;
    detail::fnd::FileStream* OpenExtStream(void* buffer, size_t size,
                                                   const char* extFilePath, void* cacheBuffer,
                                                   size_t cacheSize) const override;

private:
    const void* m_pBuffer{};
    detail::SoundArchiveFileReader m_ArchiveReader;
};
static_assert(sizeof(MemorySoundArchive) == 0x2f0);

}  // namespace nn::atk
