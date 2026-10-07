#include <nn/atk/atk_WaveSoundLoader.h>

namespace nn::atk::detail::driver {

WaveSoundLoader::~WaveSoundLoader() {
    m_Task.Wait();
    m_FreePlayerHeapTask.Wait();
}

}  // namespace nn::atk::detail::driver
