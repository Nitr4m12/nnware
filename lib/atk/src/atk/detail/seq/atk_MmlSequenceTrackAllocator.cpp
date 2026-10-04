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

}  // namespace nn::atk::detail::driver
