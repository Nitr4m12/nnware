#pragma once

#include <cstddef>
#include <cstdint>

#include <nn/os/os_ThreadTypes.h>

namespace nn::atk::detail::fnd {

class TimeSpan;

class Thread {
public:
    class ThreadMain;

    static const int64_t InvalidId{0xffffffff};

    static const int DefaultThreadPriority{16};
    static const int MinThreadPriority{0};
    static const int MaxThreadPriority{31};

    static const int StackAlignment{4096};

    using Handle = os::ThreadType;

    enum FsPriority {
        FsPriority_RealTime,
        FsPriority_Normal,
        FsPriority_Low,
    };

    enum AffinityMask : uint32_t {
        AffinityMask_CoreDefault = 0,
        AffinityMask_CoreAll = 0xffffffff,
        AffinityMask_Core0 = 1 << 0,
        AffinityMask_Core1 = 1 << 1,
        AffinityMask_Core2 = 1 << 2,
        AffinityMask_Core3 = 1 << 3,
        AffinityMask_Core4 = 1 << 4,
        AffinityMask_Core5 = 1 << 5,
        AffinityMask_Core6 = 1 << 6,
        AffinityMask_Core7 = 1 << 7,
        AffinityMask_Core8 = 1 << 8,
        AffinityMask_Core9 = 1 << 9,
        AffinityMask_Core10 = 1 << 10,
        AffinityMask_Core11 = 1 << 11,
        AffinityMask_Core12 = 1 << 12,
        AffinityMask_Core13 = 1 << 13,
        AffinityMask_Core14 = 1 << 14,
        AffinityMask_Core15 = 1 << 15,
        AffinityMask_Core16 = 1 << 16,
        AffinityMask_Core17 = 1 << 17,
        AffinityMask_Core18 = 1 << 18,
        AffinityMask_Core19 = 1 << 19,
        AffinityMask_Core20 = 1 << 20,
        AffinityMask_Core21 = 1 << 21,
        AffinityMask_Core22 = 1 << 22,
        AffinityMask_Core23 = 1 << 23,
        AffinityMask_Core24 = 1 << 24,
        AffinityMask_Core25 = 1 << 25,
        AffinityMask_Core26 = 1 << 26,
        AffinityMask_Core27 = 1 << 27,
        AffinityMask_Core28 = 1 << 28,
        AffinityMask_Core29 = 1 << 29,
        AffinityMask_Core30 = 1 << 30,
        AffinityMask_Core31 = static_cast<uint32_t>(1 << 31),
    };

    class Handler {
    public:
        virtual ~Handler() = default;
        virtual uint32_t Run(void* param) = 0;
    };
    static_assert(sizeof(Handler) == 0x8);

    struct RunArgs {
        RunArgs();

        bool IsValid() const;

        const char* name{""};
        void* stack{};
        size_t stackSize{0};
        int32_t idealCoreNumber{-1};
        AffinityMask affinityMask{AffinityMask_CoreDefault};
        int32_t priority{DefaultThreadPriority};
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        FsPriority fsPriority{FsPriority_Normal};
#endif
        void* param{};
        Handler* handler{};
    };
    static_assert(sizeof(RunArgs) == 0x38);

    enum State {
        State_NotRun,
        State_Running,
        State_Exited,
        State_Released,
    };

    Thread();
    ~Thread();

    bool Run(const RunArgs& args);

    void WaitForExit();

    void Release();

    int32_t GetPriority() const;
    FsPriority GetFsPriority() const;
    void SetPriority(int32_t value);

    State GetState() const;

    static void Sleep(const fnd::TimeSpan& timeSpan);

private:
    bool Create(Handle& handle, int64_t& id, const RunArgs& args);

    void Detach();

    void SetName(const char* name);
    void SetAffinityMask(int32_t idealCoreNumber, AffinityMask value);
    void SetFsPriority(FsPriority value);

    void Resume();
    void Join();

    bool IsTerminated() const;

    void SetState(State value);

    void OnRun();
    void OnExit();

    uint32_t m_State{State_NotRun};
    Handle m_Handle;
    int64_t m_Id{InvalidId};
    int32_t m_Priority{DefaultThreadPriority};
    FsPriority m_FsPriority;
    void* m_Param;
    Handler* m_Handler{};
    volatile bool m_IsTerminated{false};
};
static_assert(sizeof(Thread) == 0x1f0);

}  // namespace nn::atk::detail::fnd
