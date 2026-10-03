#pragma once

#include <nn/audio/audio_PerformanceMetrics.h>
#include <nn/os.h>
#include <nn/os/os_MessageQueue.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_AudioRendererPerformanceReader.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_ThreadInfoReader.h>
#include <nn/atk/fnd/os/atkfnd_CriticalSection.h>
#include <nn/atk/fnd/os/atkfnd_Thread.h>

namespace nn::atk::detail::driver {

class SoundThread : public fnd::Thread::Handler {
public:
    using ProfileFunc = void (*)(os::Tick*);

    class SoundFrameCallback {
    public:
        util::IntrusiveListNode m_Link;

        virtual ~SoundFrameCallback() = default;

        virtual void OnBeginSoundFrame() {};
        virtual void OnEndSoundFrame() {};
    };

    class PlayerCallback {
    public:
        util::IntrusiveListNode m_Link;

        virtual ~PlayerCallback() = default;

        virtual void OnUpdateFrameSoundThread() {};
        virtual void OnUpdateFrameSoundThreadWithAudioFrameFrequency() {};
        virtual void OnShutdownSoundThread() {};
    };

    static const int32_t ThreadMessageBufferSize{32};
    static const int32_t RendererEventWaitTimeoutMilliSeconds{100};

    enum Message {
        Message_HwCallback = 1 << 28,
        Message_Shutdown = 2 << 28,
        Message_ForceWakeup = 3 << 28,
    };

    static SoundThread& GetInstance();

    bool CreateSoundThread(int32_t threadPriority, void* stackBase, size_t stackSize,
                           int32_t idealCoreNumber, uint32_t affinityMask);

    void Destroy();

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    void Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                    bool isProfilingEnabled);
#elif NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                    bool isProfilingEnabled, bool isUserThreadRenderingEnabled);
#else
    void Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                    bool isProfilingEnabled, bool isDetailSoundThreadProfilerEnabled,
                    bool isUserThreadRenderingEnabled);
#endif

    void Finalize();

    void Pause(bool flag);

    uint32_t GetAxCallbackCounter() const { return m_AxCallbackCounter; }

    void UpdateLowLevelVoices();

    void FrameProcess(UpdateType updateType);
    void EffectFrameProcess();

    void RegisterSoundFrameUserCallback(SoundFrameUserCallback callback, uintptr_t arg);
    void ClearSoundFrameUserCallback();

    void RegisterThreadBeginUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    void ClearThreadBeginUserCallback();

    void RegisterThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    void ClearThreadEndUserCallback();

    void RegisterSoundThreadInfoRecorder(ThreadInfoRecorder& recorder);
    void UnregisterSoundThreadInfoRecorder(ThreadInfoRecorder& recorder);

    void RegisterSoundFrameCallback(SoundFrameCallback* callback);
    void UnregisterSoundFrameCallback(SoundFrameCallback* callback);

    void RegisterPlayerCallback(PlayerCallback* callback);
    void UnregisterPlayerCallback(PlayerCallback* callback);

    void Lock() { m_CriticalSection.Lock(); }

    void Unlock() { m_CriticalSection.Unlock(); }

    void LockAtkStateAndParameterUpdate();
    void UnlockAtkStateAndParameterUpdate();

    void RegisterProfileReader(ProfileReader& profileReader);
    void UnregisterProfileReader(ProfileReader& profileReader);

    void RegisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& performanceReader);
    void
    UnregisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& performanceReader);

    void RegisterSoundThreadUpdateProfileReader(SoundThreadUpdateProfileReader& profileReader);
    void UnregisterSoundThreadUpdateProfileReader(SoundThreadUpdateProfileReader& profileReader);

    void ForceWakeup();

    int32_t GetRendererEventWaitTimeMilliSeconds();

private:
    using SoundFrameCallbackList = util::IntrusiveList<
        SoundFrameCallback,
        util::IntrusiveListMemberNodeTraits<SoundFrameCallback, &SoundFrameCallback::m_Link>>;

    using PlayerCallbackList = util::IntrusiveList<
        PlayerCallback,
        util::IntrusiveListMemberNodeTraits<PlayerCallback, &PlayerCallback::m_Link>>;

    SoundThread();
    ~SoundThread() override;

    uint32_t Run(void* param) override;

    void RecordPerformanceInfo(audio::PerformanceInfo& src, os::Tick beginTick, os::Tick endTick,
                               uint32_t nwVoiceCount);

    void RecordUpdateProfile(const SoundThreadUpdateProfile& updateProfile);

    fnd::Thread m_Thread;
    os::MessageQueue m_BlockingQueue{m_MsgBuffer, ThreadMessageBufferSize};
    uintptr_t m_MsgBuffer[ThreadMessageBufferSize];
    uint32_t m_AxCallbackCounter;
    fnd::CriticalSection m_CriticalSection;
    fnd::CriticalSection m_UpdateAtkStateAndParameterSection;
    SoundFrameCallbackList m_SoundFrameCallbackList;
    PlayerCallbackList m_PlayerCallbackList;
    volatile SoundFrameUserCallback m_UserCallback;
    volatile uintptr_t m_UserCallbackArg;
    volatile SoundThreadUserCallback m_ThreadBeginUserCallback;
    volatile uintptr_t m_ThreadBeginUserCallbackArg;
    volatile SoundThreadUserCallback m_ThreadEndUserCallback;
    volatile uintptr_t m_ThreadEndUserCallbackArg;
    int32_t m_SoundThreadAffinityMask;
    bool m_CreateFlag{false};
    bool m_PauseFlag{false};
    os::Tick m_LastPerformanceFrameBegin{0};
    os::Tick m_LastPerformanceFrameEnd{0};
    void* m_pPerformanceFrameUpdateBuffer[3];
    size_t m_PerformanceFrameUpdateBufferSize;
    int32_t m_CurrentPerformanceFrameBufferIndex{0};
    bool m_IsProfilingEnabled{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    ProfileFunc m_pSoundThreadProfileFunc{};
#endif
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    bool m_IsUserThreadRenderingEnabled{false};
#endif
    ProfileReaderList m_ProfileReaderList;
    AudioRendererPerformanceReader* m_pAudioRendererPerformanceReader{};
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    SoundThreadInfoRecorderList m_InfoRecorderList;
    fnd::CriticalSection m_LockRecordInfo;
#endif
    SoundThreadUpdateProfile m_LastUpdateProfile;
    SoundThreadUpdateProfileReaderList m_UpdateProfileReaderList;
    fnd::CriticalSection m_LockUpdateProfile;
    std::atomic_int m_RendererEventWaitTimeMilliSeconds{0};
};
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
static_assert(sizeof(SoundThread) == 0x4c8);
#elif NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SoundThread) == 0x4f8);
#else
static_assert(sizeof(SoundThread) == 0x508);
#endif

class SoundThreadLock {
public:
    SoundThreadLock() { SoundThread::GetInstance().Lock(); }

    ~SoundThreadLock() { SoundThread::GetInstance().Unlock(); }

private:
    NN_NO_COPY(SoundThreadLock);
};

class AtkStateAndParameterUpdateLock {
public:
    AtkStateAndParameterUpdateLock() {
        SoundThread::GetInstance().LockAtkStateAndParameterUpdate();
    }

    ~AtkStateAndParameterUpdateLock() {
        SoundThread::GetInstance().UnlockAtkStateAndParameterUpdate();
    }

private:
    NN_NO_COPY(AtkStateAndParameterUpdateLock);
};

}  // namespace nn::atk::detail::driver
