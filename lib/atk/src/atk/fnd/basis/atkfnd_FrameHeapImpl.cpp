#include <nn/atk/fnd/basis/atkfnd_FrameHeapImpl.h>

namespace nn::atk::detail::fnd {

void* FrameHeapImpl::Destroy() {
    HeapBase::Finalize();
    return this;
}

}  // namespace nn::atk::detail::fnd
