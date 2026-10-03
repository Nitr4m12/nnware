#include <nn/atk/atk_MmlSequenceTrack.h>

#include <nn/atk/atk_MmlParser.h>

namespace nn::atk::detail::driver {

MmlSequenceTrack::MmlSequenceTrack() = default;

SequenceTrack::ParseResult MmlSequenceTrack::Parse(bool doNoteOn) {
    return m_pParser->Parse(this, doNoteOn);
}

}  // namespace nn::atk::detail::driver
