#pragma once

#include <nn/time.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_Config.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/detail/atk_IStreamDataDecoder.h>

namespace nn::atk {

namespace detail::driver {

class StreamSoundPlayer;

}  // namespace detail::driver

struct TaskProfile {
    struct LoadStreamBlock {
    public:
        TimeSpan GetTotalTime() const;
        os::Tick GetBeginTick() const;
        os::Tick GetEndTick() const;

#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        float GetRemainingCachePercentage() const;
        size_t GetCachedLength() const;

        detail::driver::StreamSoundPlayer* GetStreamSoundPlayer() const;
#endif

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
        void SetTick(const os::Tick& beginTick, const os::Tick& endTick);
#else
        void SetData(const os::Tick& beginTick, const os::Tick& endTick,
                     const detail::IStreamDataDecoder::CacheProfile& cacheProfile);
#endif

    private:
        uint64_t m_BeginTick;
        uint64_t m_EndTick;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        position_t m_CacheStartPosition;
        size_t m_CachedLength;
        position_t m_CacheCurrentPosition;
        detail::driver::StreamSoundPlayer* m_pPlayer;
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    static_assert(sizeof(LoadStreamBlock) == 0x10);
#else
    static_assert(sizeof(LoadStreamBlock) == 0x30);
#endif

    struct LoadOpusStreamBlock {
    public:
        TimeSpan GetTotalTime() const;
        os::Tick GetBeginTick() const;
        os::Tick GetEndTick() const;

#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        float GetRemainingCachePercentage() const;
        size_t GetCachedLength() const;
#endif

        TimeSpan GetDecodeTime() const;
        int32_t GetDecodedSampleCount() const;

        TimeSpan GetFsAccessTime() const;
        size_t GetFsReadSize() const;

#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        detail::driver::StreamSoundPlayer* GetStreamSoundPlayer() const;
#endif

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
        void SetData(const os::Tick& beginTick, const os::Tick& endTick,
                     const detail::IStreamDataDecoder::DecodeProfile& decodeProfile);
#else
        void SetData(const os::Tick& beginTick, const os::Tick& endTick,
                     const detail::IStreamDataDecoder::DecodeProfile& decodeProfile,
                     const detail::IStreamDataDecoder::CacheProfile& cacheProfile);
#endif

    private:
        uint64_t m_BeginTick;
        uint64_t m_EndTick;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        position_t m_CacheStartPosition;
        size_t m_CachedLength;
        position_t m_CacheCurrentPosition;
#endif
        uint64_t m_DecodeTick;
        uint64_t m_FsAccessTick;
        size_t m_FsReadSize;
        int32_t m_DecodedSampleCount;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        detail::driver::StreamSoundPlayer* m_pPlayer;
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    static_assert(sizeof(LoadOpusStreamBlock) == 0x30);
#else
    static_assert(sizeof(LoadOpusStreamBlock) == 0x50);
#endif

    enum TaskProfileType {
        TaskProfileType_LoadStreamBlock,
        TaskProfileType_LoadOpusStreamBlock,
    };

    TaskProfileType type;
    union {
        LoadStreamBlock loadStreamBlock;
        LoadOpusStreamBlock loadOpusStreamBlock;
    };
};
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
static_assert(sizeof(TaskProfile) == 0x38);
#else
static_assert(sizeof(TaskProfile) == 0x58);
#endif

using TaskProfileReader = AtkProfileReader<TaskProfile>;

class TaskProfileLogger {
public:
    TaskProfileLogger() = default;

    void Record(const TaskProfile& profile);

    void RegisterReader(TaskProfileReader& reader);
    void UnregisterReader(const TaskProfileReader& reader);

    void SetProfilingEnabled(bool isEnabledProfiling);

    bool IsProfilingEnabled() const { return m_IsProfilingEnabled; }

    void Finalize();

private:
    using TaskProfileReaderList = util::IntrusiveList<
        TaskProfileReader,
        util::IntrusiveListMemberNodeTraits<TaskProfileReader, &TaskProfileReader::m_List>>;

    TaskProfileReaderList m_List;
    detail::fnd::CriticalSection m_Lock;
    bool m_IsProfilingEnabled;
};
static_assert(sizeof(TaskProfileLogger) == 0x38);

}  // namespace nn::atk
