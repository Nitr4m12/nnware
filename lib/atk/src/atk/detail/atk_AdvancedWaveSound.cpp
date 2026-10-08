#include <nn/atk/detail/atk_AdvancedWaveSound.h>
#include "nn/atk/atk_DriverCommand.h"

namespace nn::atk::detail {

AdvancedWaveSound::AdvancedWaveSound(AdvancedWaveSoundInstanceManager& manager)
    : m_InstanceManager(manager) {}

AdvancedWaveSound::~AdvancedWaveSound() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool AdvancedWaveSound::Initialize()
#else
bool AdvancedWaveSound::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool result{BasicSound::Initialize()};
#else
    bool result{BasicSound::Initialize(pOutputReceiver)};
#endif

    if (!result)
        return false;

    m_pTempSpecialHandle = nullptr;
    m_IsInitialized = true;
    return true;
}

void AdvancedWaveSound::Finalize() {
    if (m_IsInitialized) {
        m_IsInitialized = false;
        m_pTempSpecialHandle = nullptr;
        BasicSound::Finalize();
        m_InstanceManager.Free(this);
    }
}

void AdvancedWaveSound::Prepare(
    const driver::AdvancedWaveSoundPlayer::PrepareParameter& parameter) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandAdvancedWaveSoundPrepare>()};
        command->id = DriverCommandId_AwsdPrepare;
        command->player = &m_PlayerInstance;
        command->parameter = parameter;

        cmdmgr.PushCommand(command);
    }
}

void AdvancedWaveSound::OnUpdatePlayerPriority() {
    int priority{CalcCurrentPlayerPriority()};
    m_InstanceManager.UpdatePriority(this, priority);
}

}  // namespace nn::atk::detail
