#include <nn/atk/atk_MmlParser.h>

namespace nn::atk::detail::driver {

namespace {

const float ModSpeedBase{1 / 2.56};

}  // anonymous namespace

uint16_t MmlParser::Read16(const uint8_t** ptr) const {
    uint16_t ret{ReadByte(ptr)};
    ret |= ReadByte(ptr) << 8;
    return ret;
}

uint32_t MmlParser::Read24(const uint8_t** ptr) const {
    uint32_t ret{ReadByte(ptr)};
    ret |= ReadByte(ptr) << 8;
    ret |= ReadByte(ptr) << 16;
    return ret;
}

}  // namespace nn::atk::detail::driver
