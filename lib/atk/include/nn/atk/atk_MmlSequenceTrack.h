#pragma once

#include <nn/atk/atk_SequenceTrack.h>

namespace nn::atk::detail::driver {

class MmlParser;
class MmlSequenceTrack : SequenceTrack {
public:
    MmlSequenceTrack();

    void SetMmlParser(const MmlParser* parser) { m_pParser = parser; }
    const MmlParser* GetMmlParser() const { return m_pParser; }

protected:
    ParseResult Parse(bool doNoteOn) override;

private:
    const MmlParser* m_pParser;
};
static_assert(sizeof(MmlSequenceTrack) == 0x1f0);

}  // namespace nn::atk::detail::driver
