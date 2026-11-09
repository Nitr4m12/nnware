#pragma once

#include <cstring>

#include <nn/atk/fnd/basis/atkfnd_Config.h>

namespace nn::atk::detail::fnd {

inline size_t strncat(char* dst, size_t dstCount, const char* src, size_t srcCount) {
    size_t length{strlen(dst)};
    if (length + srcCount < dstCount)
        length = srcCount;
    else
        length = dstCount - 1 - length;

    std::strncat(dst, src, length);
    return length;
}

}  // namespace nn::atk::detail::fnd
