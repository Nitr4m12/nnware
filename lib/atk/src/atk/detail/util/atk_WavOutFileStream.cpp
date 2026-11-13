#include <nn/atk/atk_WavOutFileStream.h>

#include <algorithm>
#include <cstring>

#include <nn/atk/atk_WavBinary.h>
#include <nn/util/util_BytePtr.h>

namespace nn::atk::detail {

WavOutFileStream::WavOutFileStream() = default;

WavOutFileStream::~WavOutFileStream() = default;

bool WavOutFileStream::Open(fnd::FileStream& stream, int channels, size_t samplesPerSec) {
    m_pFileStream = &stream;

    if (WriteHeader(channels, samplesPerSec)) {
        m_WaveDataSize = 0;
        m_IsWaveDataSizeCalculating = true;
        return true;
    }

    return false;
}

size_t WavOutFileStream::Write(const void* buf, size_t length) {
    if (!IsAvailable())
        return 0;

    size_t writtenLength{0};
    while (writtenLength < length) {
        size_t restSize{length - writtenLength};
        if (restSize > m_BufferLength) {
            fnd::FndResult result;
            size_t writtenBytes{WriteDirect(buf, restSize, &result)};
            if (result.IsFailed())
                return 0;

            writtenLength += writtenBytes;
            continue;
        }

        size_t copyLength{m_BufferLength - m_ValidBufferLength};
        copyLength = std::min(restSize, copyLength);

        std::memcpy(util::BytePtr(m_Buffer, m_ValidBufferLength).Get(),
                    util::ConstBytePtr(buf, writtenLength).Get(), copyLength);
        m_ValidBufferLength += copyLength;
        writtenLength += copyLength;
        if (m_ValidBufferLength == m_BufferLength)
            FlushBuffer();
    }

    if (m_IsWaveDataSizeCalculating)
        m_WaveDataSize += length;

    return length;
}

bool WavOutFileStream::Seek(position_t offset, fnd::FileStream::SeekOrigin origin) {
    if (!m_pFileStream->CanSeek())
        return false;

    FlushBuffer();
    return m_pFileStream->Seek(offset, origin).IsSucceeded();
}

void WavOutFileStream::SetCacheBuffer(char* buf, size_t length) {
    m_Buffer = buf;
    m_BufferLength = length;
}

bool WavOutFileStream::WriteHeader(int channels, size_t samplesPerSec) {
    char buffer[sizeof(WaveBinaryHeader) + FileIoBufferAlignment];
    void* alignedBuffer{util::BytePtr(buffer).AlignUp(FileIoBufferAlignment).Get()};

    WaveBinaryHeader* header{new (alignedBuffer) WaveBinaryHeader};

    header->riffChunk.formatType = 0x45564157;  // "WAVE"
    header->riffChunk.header.size = sizeof(WaveBinaryHeader) - sizeof(ChunkHeader);
    header->fmtChunk.bitsPerSample = 16;
    header->fmtChunk.blockAlign = channels * 2;
    header->fmtChunk.samplesPerSec = samplesPerSec;
    header->fmtChunk.formatTag = FmtChunk::FormatPcm;
    header->fmtChunk.channels = channels;
    header->fmtChunk.avgBytesPerSec = header->fmtChunk.blockAlign * header->fmtChunk.samplesPerSec;

    size_t result{Write(header, sizeof(WaveBinaryHeader))};
    return result == sizeof(WaveBinaryHeader);
}

size_t WavOutFileStream::WriteDirect(const void* buf, size_t length, fnd::FndResult* pResult) {
    if (!IsAvailable())
        return 0;

    const void* alignedBuffer{util::ConstBytePtr(buf).AlignUp(FileIoBufferAlignment).Get()};

    [[maybe_unused]] ptrdiff_t unAlignedLength{util::ConstBytePtr(buf, length).Distance(buf)};
    size_t alignedBufferLength{length};

    size_t writtenBytes{m_pFileStream->Write(alignedBuffer, alignedBufferLength, pResult)};

    if (pResult->IsFailed()) {
        char bufferForAlignment[2];
        [[maybe_unused]] char* alignedBufferForAlignment{
            util::BytePtr(bufferForAlignment).AlignUp(FileIoBufferAlignment).Get<char>(),
        };

        return 0;
    }

    return writtenBytes;
}

bool WavOutFileStream::UpdateRiffChunkSize() {
    if (!IsAvailable())
        return false;

    const position_t riffChunkSizePosition{offsetof(WaveBinaryHeader, riffChunk) + 4};
    if (!Seek(riffChunkSizePosition, fnd::FileStream::SeekOrigin_Begin))
        return false;

    char buffer[5];
    void* alignedBuffer{util::BytePtr(buffer).AlignUp(FileIoBufferAlignment).Get()};

    fnd::PcBinU32& size{reinterpret_cast<RiffChunk*>(buffer - 4)->header.size};
    size = CalcRiffChunkSize(m_WaveDataSize);

    fnd::FndResult result;
    WriteDirect(alignedBuffer, riffChunkSizePosition, &result);

    return !result.IsFailed();
}

bool WavOutFileStream::UpdateDataChunkSize() {
    if (!IsAvailable())
        return false;

    const int32_t dataChunkSizePosition{offsetof(WaveBinaryHeader, dataChunk) + 4};
    if (!Seek(dataChunkSizePosition, fnd::FileStream::SeekOrigin_Begin))
        return false;

    uint8_t buffer[5];
    void* alignedBuffer{util::BytePtr(buffer).AlignUp(FileIoBufferAlignment).Get()};

    fnd::PcBinU32& size{reinterpret_cast<DataChunk*>(buffer - 4)->header.size};
    size = m_WaveDataSize;

    fnd::FndResult result;
    WriteDirect(alignedBuffer, 4, &result);

    return !result.IsFailed();
}

size_t WavOutFileStream::CalcRiffChunkSize(size_t dataSize) {
    return dataSize + sizeof(RiffChunk) + sizeof(FmtChunk);
}

size_t WavOutFileStream::FlushBuffer() {
    size_t length{m_ValidBufferLength};
    m_ValidBufferLength = 0;

    if (length == 0)
        return 0;

    fnd::FndResult result;
    length = WriteDirect(m_Buffer, length, &result);

    return length;
}

}  // namespace nn::atk::detail
