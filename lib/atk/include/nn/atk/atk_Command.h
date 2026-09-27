#pragma once

#include <cstdint>

namespace nn::atk::detail {

struct Command {
    Command* next;
    uint32_t id;
    uint32_t tag;
    std::uintptr_t memory_next;
};
static_assert(sizeof(Command) == 0x18);

}  // namespace nn::atk::detail
