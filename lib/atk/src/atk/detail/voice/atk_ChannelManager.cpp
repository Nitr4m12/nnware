#include <nn/atk/atk_ChannelManager.h>

namespace nn::atk::detail::driver {

ChannelManager& ChannelManager::GetInstance() {
    static ChannelManager instance;
    return instance;
}

ChannelManager::ChannelManager() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t ChannelManager::GetRequiredMemSize(int32_t channelCount) {
    return (static_cast<size_t>(channelCount * 2) / 2) * (sizeof(Channel) * 2) + 0x40;
}

void ChannelManager::Initialize(void* mem, size_t memSize) {
    if (!m_IsInitialized) {
        m_ChannelCount = m_Pool.Create(mem, memSize, 64);
        m_IsInitialized = true;
    }
}
#else
size_t ChannelManager::GetObjectSize(const SoundInstanceConfig& config) {
    size_t result{sizeof(Channel)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += AdditionalParamBufferSize;
        result += sizeof(OutputAdditionalParam);
        result = (result + (ChannelPool::DefaultAlignment - 1)) &  // align down
                 -ChannelPool::DefaultAlignment;
    }

    return result;
}

size_t ChannelManager::GetRequiredMemSize(int32_t channelCount, const SoundInstanceConfig& config) {
    return GetObjectSize(config) * channelCount + 0x70;
}

// TODO: void ChannelManager::Initialize;
#endif

void ChannelManager::Finalize() {
    if (!m_IsInitialized)
        return;

    for (auto itr{m_ChannelList.begin()}; itr != m_ChannelList.end();) {
        Channel* channel{&*itr};
        ++itr;

        channel->Stop();
        channel->CallChannelCallback(Channel::ChannelCallbackStatus_Cancel);
        Free(channel);
    }

#if NN_WARE_VER > NN_MAKE_VER(4, 0, 0)
    if (m_pAdditionalParamPool != nullptr) {
        m_pAdditionalParamPool->Destroy();
        m_pAdditionalParamPool = nullptr;
    }

    if (m_pAdditionalParamBufferPool != nullptr) {
        m_pAdditionalParamBufferPool->Destroy();
        m_pAdditionalParamBufferPool = nullptr;
    }
#endif

    m_Pool.Destroy();
    m_IsInitialized = false;
}

void ChannelManager::Free(Channel* channel) {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_pAdditionalParamBufferPool != nullptr && m_pAdditionalParamPool != nullptr) {
        OutputAdditionalParam* pAdditionalParam{channel->GetTvAdditionalParamAddr()};
        m_pAdditionalParamBufferPool->Free(pAdditionalParam->GetBufferAddr());
        m_pAdditionalParamPool->Free(pAdditionalParam);
    }
#endif

    m_ChannelList.erase(m_ChannelList.iterator_to(*channel));
    m_Pool.Free(channel);
}

}  // namespace nn::atk::detail::driver
