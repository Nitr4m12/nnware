#pragma once

#include <nn/nn_TimeSpan.h>

#include <nn/atk/atk_SoundThread.h>

namespace nn::atk::detail {

template <typename Instance>
class LoaderManager : public driver::SoundThread::SoundFrameCallback {
public:
    using List = util::IntrusiveList<
        Instance, util::IntrusiveListMemberNodeTraits<Instance, &Instance::m_LinkForLoaderManager>>;
    using Iterator = typename List::iterator;

    LoaderManager() = default;
    ~LoaderManager() override = default;

    static size_t GetRequiredMemSize(int count) { return sizeof(Instance) * count; }

    int Create(void* buffer, size_t size) {
        char* ptr{reinterpret_cast<char*>(buffer)};
        const int objectCount{static_cast<int>(size / sizeof(Instance))};

        for (int i{0}; i < objectCount; ++i) {
            Instance* instance{new (ptr) Instance};
            m_FreeList.push_back(*instance);

            ptr += sizeof(Instance);
        }

        m_pBuffer = buffer;
        m_BufferSize = size;

        return objectCount;
    }

    void Destroy() {
        while (!m_FreeReqList.empty())
            os::SleepThread(TimeSpanType{5000000});

        char* ptr{static_cast<char*>(m_pBuffer)};
        const int objectCount{static_cast<int>(m_BufferSize / sizeof(Instance))};

        for (int i{0}; i < objectCount; ++i) {
            Instance* instance{reinterpret_cast<Instance*>(ptr)};
            instance->~Instance();

            ptr += sizeof(Instance);
        }

        m_FreeList.clear();
    }

    Instance* Alloc() {
        if (m_FreeList.empty())
            return nullptr;

        Instance* instance{&m_FreeList.front()};
        m_FreeList.pop_front();

        return instance;
    }

    void Free(Instance* instance) {
        if (instance->IsInUse()) {
            m_FreeReqList.push_back(*instance);
        } else {
            instance->Finalize();
            m_FreeList.push_back(*instance);
        }
    }

    void UpdateFreeReqList() {
        for (auto itr{m_FreeReqList.begin()}; itr != m_FreeReqList.end();) {
            auto curItr{itr++};
            if (!curItr->IsInUse()) {
                curItr->Finalize();
                m_FreeReqList.erase(curItr);
                m_FreeList.push_back(*curItr);
            }
        }
    }

    int GetFreeCount() const { return m_FreeList.size(); }

    void OnBeginSoundFrame() override { UpdateFreeReqList(); }

private:
    void* m_pBuffer{};
    size_t m_BufferSize{0};
    List m_FreeList;
    List m_FreeReqList;
};

}  // namespace nn::atk::detail
