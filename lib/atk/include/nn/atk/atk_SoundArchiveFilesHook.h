#pragma once

#include <cstddef>
#include <cstdint>

#include <nn/atk/fnd/io/atkfnd_FileStream.h>

namespace nn::atk::detail {

class SoundArchiveFilesHook {
public:
    static const char ItemTypeStreamSound[];
    static const char ItemTypeWaveSound[];
    static const char ItemTypeSequenceSound[];

    static const char FileTypeStreamBinary[];
    static const char FileTypeWaveSoundBinary[];
    static const char FileTypeSequenceBinary[];
    static const char FileTypeBankBinary[];
    static const char FileTypeWaveArchiveBinary[];
    static const char FileTypeStreamPrefetchBinary[];

    bool GetIsEnable() const { return m_IsEnable; }
    void SetIsEnable(bool value) { m_IsEnable = value; }

    bool IsTargetItem(const char* itemLabel);

    void Lock();
    void Unlock();

    fnd::FileStream* OpenFile(void* buffer, size_t bufferLength, void* cacheBuffer,
                              size_t cacheBufferLength, const char* itemLabel,
                              const char* fileType) {
        if (!m_IsEnable)
            return nullptr;

        return OpenFileImpl(buffer, bufferLength, cacheBuffer, cacheBufferLength, itemLabel,
                            fileType);
    }

    const void* GetFileAddress(const char* itemLabel, const char* itemType, const char* fileType,
                               uint32_t fileIndex) {
        if (!m_IsEnable)
            return nullptr;

        return GetFileAddressImpl(itemLabel, itemType, fileType, fileIndex);
    }

protected:
    // XXX: these are not part of debug symbols, but are here
    // because this class needs virtual functions. Names are pure
    // guesses

    virtual void Impl0();
    virtual void Impl1();
    virtual void Impl2();
    virtual void Impl3();
    virtual void Impl4();
    virtual fnd::FileStream* OpenFileImpl(void* buffer, size_t bufferLength, void* cacheBuffer,
                                          size_t cacheBufferLength, const char* itemLabel,
                                          const char* fileType);
    virtual const void* GetFileAddressImpl(const char* itemLabel, const char* itemType,
                                           const char* fileType, uint32_t fileIndex);

private:
    bool m_IsEnable;
};

}  // namespace nn::atk::detail
