#pragma once

#include <atomic>

#include <nn/audio/audio_Common.h>
#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_HardwareManager.h>

namespace nn::atk {

struct TimeRange {
    os::Tick begin{0};
    os::Tick end{0};
};
static_assert(sizeof(TimeRange) == 0x10);

struct SoundProfile {
    TimeRange nwFrameProcess;
    TimeRange mainMixProcess;
    TimeRange finalMixProcess;
    TimeRange voiceProcess;
    TimeRange sinkProcess;
    TimeRange circularBufferSinkProcess;
    TimeRange rendererFrameProcess;
    uint32_t totalVoiceCount;
    uint32_t rendererVoiceCount;
    uint32_t nwVoiceCount;
    uint64_t nwFrameProcessTick;
    TimeRange _additionalSubMixProcess;
    TimeRange _voiceProcessTable[detail::driver::HardwareManager::AtkVoiceCountMax];
    audio::NodeId _voiceIdTable[detail::driver::HardwareManager::AtkVoiceCountMax];
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SoundProfile) == 0x818);
#else
static_assert(sizeof(SoundProfile) == 0xf98);
#endif

struct SoundThreadUpdateProfile {
    TimeRange soundThreadProcess;
    TimeRange _updateLowLevelVoiceProcess;
    TimeRange _updateRendererProcess;
    TimeRange _waitRendererEventProcess;
    TimeRange _userEffectFrameProcess;
    TimeRange _frameProcess;
};
static_assert(sizeof(SoundThreadUpdateProfile) == 0x60);

class ProfileReader {
public:
    ProfileReader();

    int32_t Read(SoundProfile* profile, int32_t maxCount);

    void Record(const SoundProfile& src);

    util::IntrusiveListNode m_Link;

private:
    SoundProfile m_ProfileBuffer[StreamDataLoadTaskMax];
    int32_t m_ProfileBufferRead{0};
    int32_t m_ProfileBufferWrite{0};
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(ProfileReader) == 0x10318);
#else
static_assert(sizeof(ProfileReader) == 0x1f318);
#endif

using ProfileReaderList =
    util::IntrusiveList<ProfileReader,
                        util::IntrusiveListMemberNodeTraits<ProfileReader, &ProfileReader::m_Link>>;

struct TaskProfile;

template <typename TProfile>
class AtkProfileReader {
public:
    AtkProfileReader() = default;

    size_t GetRequiredMemorySize(int32_t alignment);

    size_t GetRequirdMemorySize(int32_t alignment);

    void Initialize(void*, size_t, int);
    void Finalize();

    int Read(TProfile* pOutProfile, int readSize);

    void Record(const TProfile& profile) {
        if (m_ReadableCount < m_ProfileCount) {
            m_pProfile[m_RecordIndex++] = profile;

            if (m_RecordIndex == m_ProfileCount)
                m_RecordIndex = 0;

            ++m_ReadableCount;
        }
    }

    bool IsInitialized() const { return m_IsInitialized; }

    util::IntrusiveListNode m_List;

private:
    NN_NO_COPY(AtkProfileReader);

    bool m_IsInitialized;
    TProfile* m_pProfile;
    int32_t m_ProfileCount;
    int32_t m_RecordIndex;
    int32_t m_ReadIndex;
    std::atomic_int m_ReadableCount;
};

using SoundThreadUpdateProfileReader = AtkProfileReader<SoundThreadUpdateProfile>;
using SoundThreadUpdateProfileReaderList = util::IntrusiveList<
    SoundThreadUpdateProfileReader,
    util::IntrusiveListMemberNodeTraits<SoundThreadUpdateProfileReader,
                                        &SoundThreadUpdateProfileReader::m_List>>;

}  // namespace nn::atk
