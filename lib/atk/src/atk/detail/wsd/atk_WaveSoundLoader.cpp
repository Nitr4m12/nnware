#include <nn/atk/atk_WaveSoundLoader.h>

#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_TaskManager.h>

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
    m_Data.Initialize();
    m_IsLoadSuccess = false;
}

void WaveSoundLoader::FreePlayerHeapTask::Initialize() {
    InitializeStatus();
}

void WaveSoundLoader::Finalize() {
    m_FreePlayerHeapTask.m_pPlayerHeap = m_Task.m_pPlayerHeap;
    TaskManager::GetInstance().AppendTask(&m_FreePlayerHeapTask, TaskManager::TaskPriority_Middle);
}

bool WaveSoundLoader::TryWait() {
    if (!m_Task.TryAllocPlayerHeap())
        return false;

    Task::Status status{m_Task.GetStatus()};

    switch (status) {
    case Task::Status_Free:
        TaskManager::GetInstance().AppendTask(&m_Task, TaskManager::TaskPriority_Middle);
        break;
    case Task::Status_Append:
    case Task::Status_Execute:
        break;
    case Task::Status_Done:
    case Task::Status_Cancel:
        return true;
    }

    return false;
}

bool WaveSoundLoader::DataLoadTask::TryAllocPlayerHeap() {
    if (m_pPlayerHeap == nullptr) {
        m_pPlayerHeap = m_Arg.soundPlayer->detail_AllocPlayerHeap();
        if (m_pPlayerHeap == nullptr)
            return false;
    }

    return true;
}

bool WaveSoundLoader::IsInUse() {
    return !m_Task.TryWait() || !m_FreePlayerHeapTask.TryWait();
}

void WaveSoundLoader::DataLoadTask::Execute([[maybe_unused]] TaskProfileLogger& logger) {
    m_pPlayerHeapDataManager->Initialize(m_Arg.soundArchive);

    if (m_Arg.loadInfoWsd.address == nullptr) {
        SoundArchive::ItemId soundId{m_Arg.loadInfoWsd.itemId};
        if (soundId != SoundArchive::InvalidId) {
            if (!m_pPlayerHeapDataManager->LoadData(soundId, m_pPlayerHeap,
                                                    SoundArchiveLoader::LoadFlag_Wsd, 0)) {
                m_pPlayerHeap->SetState(PlayerHeap::State_TaskFinished);
                m_IsLoadSuccess = false;
                return;
            }

            m_Arg.loadInfoWsd.address =
                m_pPlayerHeapDataManager->detail_GetFileAddressByItemId(soundId);
        }
    }

    const void* waveFile{Util::GetWaveFileOfWaveSound(
        m_Arg.loadInfoWsd.address, m_Arg.index, *m_Arg.soundArchive, *m_Arg.soundDataManager)};

    if (waveFile == nullptr) {
        if (!m_pPlayerHeapDataManager->detail_LoadWaveArchiveByWaveSoundFile(
                m_Arg.loadInfoWsd.address, m_Arg.index, m_pPlayerHeap)) {
            m_pPlayerHeap->SetState(PlayerHeap::State_TaskFinished);
            m_IsLoadSuccess = false;
            return;
        }

        waveFile = Util::GetWaveFileOfWaveSound(m_Arg.loadInfoWsd.address, m_Arg.index,
                                                *m_Arg.soundArchive, *m_pPlayerHeapDataManager);
    }

    m_Data.wsdFile = m_Arg.loadInfoWsd.address;
    m_Data.waveFile = waveFile;

    m_pPlayerHeap->SetState(PlayerHeap::State_TaskFinished);
    m_IsLoadSuccess = true;
}

void WaveSoundLoader::FreePlayerHeapTask::Execute([[maybe_unused]] TaskProfileLogger& logger) {
    if (m_pPlayerHeap != nullptr) {
        m_pPlayerHeap->Clear();
        m_Arg.soundPlayer->detail_FreePlayerHeap(m_pPlayerHeap);
    }

    m_pPlayerHeapDataManager->Finalize();
}

}  // namespace nn::atk::detail::driver
