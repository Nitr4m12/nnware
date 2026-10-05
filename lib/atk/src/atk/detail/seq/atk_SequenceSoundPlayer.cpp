#include <nn/atk/atk_SequenceSoundPlayer.h>
#include "nn/atk/atk_BasicSoundPlayer.h"
#include "nn/atk/atk_DisposeCallbackManager.h"
#include "nn/atk/atk_TaskManager.h"

namespace {

const uint32_t IntervalMsecNumerator{5 << 16};
const uint32_t IntervalMsecDenominator{1 << 16};

}  // anonymous namespace

namespace nn::atk::detail::driver {

void SequenceSoundPlayer::InitSequenceSoundPlayer() {
    for (int variableNo{0}; variableNo < GlobalVariableCount; ++variableNo)
        m_GlobalVariable[variableNo] = VariableDefaultValue;
}

SequenceSoundPlayer::SequenceSoundPlayer() {
    for (int varNo{0}; varNo < PlayerVariableCount; ++varNo)
        m_LocalVariable[varNo] = VariableDefaultValue;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        m_pTracks[trackNo] = nullptr;
}

SequenceSoundPlayer::~SequenceSoundPlayer() {
    Finalize();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void SequenceSoundPlayer::Initialize()
#else
void SequenceSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    BasicSoundPlayer::Initialize();
#else
    BasicSoundPlayer::Initialize(pOutputReceiver);
#endif

    SetPauseFlag(false);
    m_ReleasePriorityFixFlag = false;

    SetStartedFlag(false);
    SetActiveFlag(false);

    m_TickFraction = 0.0f;
    m_SkipTickCounter = 0;
    m_SkipTimeCounter = 0.0f;

    m_PanRange = 1.0f;
    m_TempoRatio = 1.0f;

    m_DelayCount = 0;
    m_TickCounter = 0;

    m_UpdateType = UpdateType_AudioFrame;

    m_SequenceUserprocCallback = nullptr;
    m_pSequenceUserprocCallbackArg = nullptr;

    m_ParserParam.tempo = DefaultTempo;
    m_ParserParam.volume.InitValue(127);
    m_ParserParam.priority = 64;
    m_ParserParam.timebase = DefaultTimebase;
    m_ParserParam.callback = nullptr;

    for (int varNo{0}; varNo < PlayerVariableCount; ++varNo)
        m_LocalVariable[varNo] = VariableDefaultValue;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        m_pTracks[trackNo] = nullptr;

    m_IsInitialized = true;
    m_IsRegisterPlayerCallback = false;
}

void SequenceSoundPlayer::FreeLoader() {
    if (m_pLoader != nullptr) {
        m_pLoaderManager->Free(m_pLoader);
        m_pLoader = nullptr;
    }
}

void SequenceSoundLoader::Finalize() {
    m_FreePlayerHeapTask.m_pPlayerHeap = m_Task.m_pPlayerHeap;
    TaskManager::GetInstance().AppendTask(&m_FreePlayerHeapTask, TaskManager::TaskPriority_Middle);
}

bool SequenceSoundLoader::IsInUse() {
    return !m_Task.TryWait() || !m_FreePlayerHeapTask.TryWait();
}

}  // namespace nn::atk::detail::driver
