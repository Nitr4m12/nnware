#pragma once

#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_InstancePool.h>

namespace nn::atk::detail::driver {

class ChannelManager {
public:
    static ChannelManager& GetInstance();

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size_t GetRequiredMemSize(int32_t channelCount);
    void Initialize(void* mem, size_t memSize);
#else
    size_t GetObjectSize(const SoundInstanceConfig& config);
    size_t GetRequiredMemSize(int32_t channelCount, const SoundInstanceConfig& config);

    void Initialize(void* mem, size_t memSize, int32_t channelCount,
                    const SoundInstanceConfig& config);
#endif
    void Finalize();

    Channel* Alloc();
    void Free(Channel* channel);

    void UpdateAllChannel();
    void UpdateAudioFrameChannel();

private:
    using ChannelPool = InstancePool<Channel>;
    using ChannelList =
        util::IntrusiveList<Channel,
                            util::IntrusiveListMemberNodeTraits<Channel, &Channel::m_Link>>;

    ChannelManager();

    ChannelPool m_Pool;
    ChannelList m_ChannelList;
    bool m_IsInitialized{false};
    int32_t m_ChannelCount{0};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    InstancePool<OutputAdditionalParam>* m_pAdditionalParamPool{};
    BufferPool* m_pAdditionalParamBufferPool{};
    size_t m_AdditionalParamBufferSizePerChannel{0};
    SoundInstanceConfig m_SoundInstanceConfig;
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(ChannelManager) == 0x30);
#else
static_assert(sizeof(ChannelManager) == 0x50);
#endif

}  // namespace nn::atk::detail::driver
