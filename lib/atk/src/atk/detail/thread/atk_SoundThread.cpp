#include <nn/atk/atk_SoundThread.h>

namespace nn::atk::detail::driver {

SoundThread& SoundThread::GetInstance() {
    static SoundThread instance;
    return instance;
}

SoundThread::SoundThread() = default;

SoundThread::~SoundThread() = default;

bool SoundThread::CreateSoundThread(int32_t threadPriority, void* stackBase, size_t stackSize,
                                    int32_t idealCoreNumber, uint32_t affinityMask) {
    m_SoundThreadAffinityMask = affinityMask;

    fnd::Thread::RunArgs args;
    args.name = "nn::atk::detail::driver::SoundThread";
    args.stack = stackBase;
    args.stackSize = stackSize;
    args.idealCoreNumber = idealCoreNumber;
    args.affinityMask = static_cast<fnd::Thread::AffinityMask>(affinityMask);
    args.priority = threadPriority;
    args.param = nullptr;
    args.handler = this;

    m_CreateFlag = m_Thread.Run(args);

    return m_CreateFlag;
}

}  // namespace nn::atk::detail::driver
