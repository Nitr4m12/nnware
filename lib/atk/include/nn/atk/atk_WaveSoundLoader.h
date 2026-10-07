#pragma once

#include <nn/atk/atk_LoaderManager.h>
#include <nn/atk/atk_PlayerHeap.h>
#include <nn/atk/atk_PlayerHeapDataManager.h>
#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_Task.h>

namespace nn::atk::detail::driver {

class WaveSoundLoader;
using WaveSoundLoaderManager = LoaderManager<WaveSoundLoader>;

class WaveSoundLoader {
public:
    struct LoadInfo {
        const SoundArchive* soundArchive;
        const SoundDataManager* soundDataManager;
        const LoadItemInfo* loadInfoWsd;
        SoundPlayer* soundPlayer;

        LoadInfo(const SoundArchive* arc, const SoundDataManager* mgr, const LoadItemInfo* wsd,
                 SoundPlayer* player)
            : soundArchive{arc}, soundDataManager{mgr}, loadInfoWsd{wsd}, soundPlayer{player} {}
    };
    static_assert(sizeof(LoadInfo) == 0x20);

    struct Data {
        const void* wsdFile{};
        const void* waveFile{};

        Data() = default;

        void Initialize() {
            wsdFile = nullptr;
            waveFile = nullptr;
        }
    };
    static_assert(sizeof(Data) == 0x10);

    struct Arg {
        const SoundArchive* soundArchive;
        const SoundDataManager* soundDataManager;
        SoundPlayer* soundPlayer;
        LoadItemInfo loadInfoWsd;
        int32_t index;

        Arg() = default;
    };
    static_assert(sizeof(Arg) == 0x30);

    class DataLoadTask : public Task {
    public:
        void Initialize();
        void Execute(TaskProfileLogger& logger) override;
        bool TryAllocPlayerHeap();

        Arg m_Arg;
        Data m_Data;
        PlayerHeap* m_pPlayerHeap;
        PlayerHeapDataManager* m_pPlayerHeapDataManager;
        bool m_IsLoadSuccess;
        uint8_t m_Padding[3];
    };
    static_assert(sizeof(DataLoadTask) == 0xa0);

    class FreePlayerHeapTask : public Task {
    public:
        void Initialize();
        void Execute(TaskProfileLogger& logger) override;

        Arg m_Arg;
        PlayerHeap* m_pPlayerHeap;
        PlayerHeapDataManager* m_pPlayerHeapDataManager;
    };

    WaveSoundLoader() = default;
    ~WaveSoundLoader();

    bool IsInUse();

    void Initialize(const Arg& arg);
    void Finalize();

    bool TryWait();

    bool IsLoadSuccess() const { return m_Task.m_IsLoadSuccess; }

    const void* GetWsdFile() const { return m_Task.m_Data.wsdFile; }
    const void* GetWaveFile() const { return m_Task.m_Data.waveFile; }

private:
    DataLoadTask m_Task;
    FreePlayerHeapTask m_FreePlayerHeapTask;
    PlayerHeapDataManager m_PlayerHeapDataManager;

public:
    util::IntrusiveListNode m_LinkForLoaderManager;
};

}  // namespace nn::atk::detail::driver
