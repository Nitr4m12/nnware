#include <nn/atk/atk_WaveSoundLoader.h>
#include "nn/atk/atk_TaskManager.h"

namespace nn::atk::detail::driver {

WaveSoundLoader::~WaveSoundLoader() {
    m_Task.Wait();
    m_FreePlayerHeapTask.Wait();
}

void WaveSoundLoader::Initialize(const Arg& arg) {
    m_Task.Wait();
    m_FreePlayerHeapTask.Wait();

    m_Task.Initialize();
    m_Task.m_Arg = arg;
    m_Task.m_pPlayerHeap = nullptr;
    m_Task.m_pPlayerHeapDataManager = &m_PlayerHeapDataManager;

    m_FreePlayerHeapTask.Initialize();
    m_FreePlayerHeapTask.m_Arg = arg;
    m_FreePlayerHeapTask.m_pPlayerHeap = nullptr;
    m_FreePlayerHeapTask.m_pPlayerHeapDataManager = &m_PlayerHeapDataManager;
}

void WaveSoundLoader::DataLoadTask::Initialize() {
    InitializeStatus();
    m_IsLoadSuccess = false;
    m_Data.wsdFile = nullptr;
    m_Data.waveFile = nullptr;
}

void WaveSoundLoader::FreePlayerHeapTask::Initialize() {
    InitializeStatus();
}

void WaveSoundLoader::Finalize() {
    m_FreePlayerHeapTask.m_pPlayerHeap = m_Task.m_pPlayerHeap;
    TaskManager::GetInstance().AppendTask(&m_FreePlayerHeapTask, TaskManager::TaskPriority_Middle);
}

}  // namespace nn::atk::detail::driver
