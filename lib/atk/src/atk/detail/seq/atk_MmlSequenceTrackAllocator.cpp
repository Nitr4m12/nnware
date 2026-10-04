#include <nn/atk/atk_MmlSequenceTrackAllocator.h>

namespace nn::atk::detail::driver {

SequenceTrack* MmlSequenceTrackAllocator::AllocTrack(SequenceSoundPlayer* player) {
    MmlSequenceTrack* track{m_TrackPool.Alloc()};

    if (track == nullptr)
        return nullptr;

    track->SetSequenceSoundPlayer(player);
    track->SetMmlParser(m_pParser);
    return track;
}

void MmlSequenceTrackAllocator::FreeTrack(SequenceTrack* track) {
    track->SetSequenceSoundPlayer(nullptr);
    m_TrackPool.Free(static_cast<MmlSequenceTrack*>(track));
}

int32_t MmlSequenceTrackAllocator::Create(void* buffer, size_t size) {
    return m_TrackPool.Create(buffer, size);
}

}  // namespace nn::atk::detail::driver
