#pragma once

#include <nn/atk/atk_InstancePool.h>
#include <nn/atk/atk_MmlParser.h>
#include <nn/atk/atk_SequenceTrackAllocator.h>

namespace nn::atk::detail::driver {

class MmlSequenceTrackAllocator : public SequenceTrackAllocator {
public:
    using MmlSequenceTrackPool = InstancePool<MmlSequenceTrack>;

    MmlSequenceTrackAllocator(MmlParser* parser) : m_pParser(parser) {};

    void SetMmlParser(MmlParser* parser) { m_pParser = parser; }
    MmlParser* GetMmlParser() { return m_pParser; }

    SequenceTrack* AllocTrack(SequenceSoundPlayer* player) override;
    void FreeTrack(SequenceTrack* track) override;

    int32_t GetAllocatableTrackCount() const override { return m_TrackPool.Count(); }

    int32_t Create(void* buffer, size_t size);

    void Destroy();

private:
    MmlParser* m_pParser;
    MmlSequenceTrackPool m_TrackPool;
};
static_assert(sizeof(MmlSequenceTrackAllocator) == 0x28);

}  // namespace nn::atk::detail::driver
