#pragma once

#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_Config.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/fnd/io/atkfnd_FileStream.h>

namespace nn::atk::detail {
namespace driver {

class StreamSoundPlayer;

}  // namespace driver

class IStreamDataDecoder {
public:
    enum DecodeType { DecodeType_Normal, DecodeType_Loop, DecodeType_Idling, DecodeType_Count };

    struct DataInfo {
        int32_t channelCount;
        int32_t sampleRate;
        int32_t blockSampleCount;
        size_t blockSize;
    };
    static_assert(sizeof(DataInfo) == 0x18);

    struct DecodeProfile {
        os::Tick decodeTick;
        int32_t decodedSampleCount;
        os::Tick fsAccessTick;
        size_t fsReadSize;
    };
    static_assert(sizeof(DecodeProfile) == 0x20);

    struct CacheProfile {
        position_t cacheStartPosition{0};
        size_t cachedLength{0};
        position_t cacheCurrentPosition{0};
        driver::StreamSoundPlayer* player{};
    };
    static_assert(sizeof(CacheProfile) == 0x20);

    virtual ~IStreamDataDecoder();
    virtual bool ReadDataInfo(DataInfo* info, fnd::FileStream* pStream);
    virtual void PrepareStreamData(fnd::FileStream* pStream);
    virtual bool DecodeStreamData(int16_t** pOutBufferAddresses, fnd::FileStream* pStream,
                                  int channelCount, DecodeType decodeType);
};

class IStreamDataDecoderManager {
public:
    // XXX: these are not part of debug symbols, but are here
    // because this class needs virtual functions. Names are pure
    // guesses

    virtual ~IStreamDataDecoderManager() = default;
    virtual IStreamDataDecoder* AllocImpl();
    virtual void FreeImpl(IStreamDataDecoder* pStreamDataDecoder);
    virtual StreamFileType GetStreamFileTypeImpl() const;
    virtual DecodeMode GetDecodeModeImpl() const;

    util::IntrusiveListNode m_Link;
};

}  // namespace nn::atk::detail
