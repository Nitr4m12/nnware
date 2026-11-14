#include <nn/atk/atk_OutputMixer.h>

#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
#include <nn/atk/fnd/basis/atkfnd_WorkBufferAllocator.h>
#endif

namespace nn::atk {

OutputMixer::OutputMixer() = default;

size_t OutputMixer::GetRequiredMemorySize(int32_t bus, bool isEffectEnabled) {
    size_t result{0};
    if (!isEffectEnabled)
        return 0;

    result = bus;
    result *= 32;

    return result;
}

void OutputMixer::Initialize(int32_t bus, bool isEffectEnabled, void* buffer,
                             [[maybe_unused]] size_t bufferSize) {
    if (isEffectEnabled) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        uint8_t* ptr{static_cast<uint8_t*>(buffer)};
        m_pEffectList = reinterpret_cast<EffectList*>(ptr);
#else
        detail::fnd::WorkBufferAllocator allocator{buffer, bufferSize};
        m_pEffectList = allocator.Allocate<EffectList>(bus);
#endif
        for (int32_t i{0}; i < bus; ++i)
            new (&m_pEffectList[i]) EffectList;

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        ptr += bus * sizeof(EffectList);
        m_pEffectAuxList = reinterpret_cast<EffectAuxList*>(ptr);
#else
        m_pEffectAuxList = allocator.Allocate<EffectAuxList>(bus);
#endif
        for (int32_t i{0}; i < bus; ++i)
            new (&m_pEffectAuxList[i]) EffectList;
    }

    m_IsEffectEnabled = isEffectEnabled;
}

void OutputMixer::Finalize() {
    m_pEffectList = nullptr;
    m_pEffectAuxList = nullptr;
    m_IsEffectEnabled = false;
}

}  // namespace nn::atk
