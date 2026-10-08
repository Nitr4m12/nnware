#include <nn/atk/atk_WaveSound.h>

#include <algorithm>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_WaveFileReader.h>

namespace nn::atk::detail {

WaveSound::WaveSound(WaveSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool WaveSound::Initialize()
#else
bool WaveSound::Initialize(OutputReceiver* pOutputReceiver)
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
    m_pWaveFile = nullptr;
    m_WaveType = WaveType_Invalid;
    m_InitializeFlag = true;
    m_IsCalledPrepare = false;
    m_ChannelCount = 0;
    return true;
}

void WaveSound::Finalize() {
    if (m_InitializeFlag) {
        m_InitializeFlag = false;
        m_IsCalledPrepare = false;
        m_ChannelCount = 0;
        BasicSound::Finalize();
        m_Manager.Free(this);
    }
}

void WaveSound::Prepare(const void* wsdFile, const void* waveFile,
                        const driver::WaveSoundPlayer::StartInfo& startInfo, int8_t waveType) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandWaveSoundPrepare>()};
        command->id = DriverCommandId_WsdPrepare;
        command->player = &m_PlayerInstance;
        command->startInfo = startInfo;
        command->arg.wsdFile = wsdFile;
        command->arg.waveFile = waveFile;
        command->arg.waveType = waveType;

        cmdmgr.PushCommand(command);
    }

    m_WaveType = waveType;
    m_pWaveFile = waveFile;
    m_IsCalledPrepare = true;

    WaveFileReader reader{waveFile, waveType};
    WaveInfo waveInfo;

    if (!reader.ReadWaveInfo(&waveInfo, nullptr))
        return;

    m_ChannelCount = std::min(waveInfo.channelCount, 2);
}

void WaveSound::RegisterDataLoadTask(const driver::WaveSoundLoader::LoadInfo& loadInfo,
                                     const driver::WaveSoundPlayer::StartInfo& startInfo) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandWaveSoundLoad>()};
        command->id = DriverCommandId_WsdLoad;
        command->player = &m_PlayerInstance;
        command->startInfo = startInfo;
        command->arg.soundDataManager = loadInfo.soundDataManager;
        command->arg.soundArchive = loadInfo.soundArchive;
        command->arg.soundPlayer = loadInfo.soundPlayer;
        command->arg.loadInfoWsd = *loadInfo.loadInfoWsd;
        command->arg.index = startInfo.index;

        cmdmgr.PushCommand(command);
    }
}

void WaveSound::SetChannelPriority(int32_t priority) {
    uint8_t uint8_t_prio{static_cast<uint8_t>(priority)};
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandWaveSoundChannelPrio>()};
        command->id = DriverCommandId_WsdChannelPrio;
        command->player = &m_PlayerInstance;
        command->priority = uint8_t_prio;

        cmdmgr.PushCommand(command);
    }
}

}  // namespace nn::atk::detail
