#pragma once

#include <atomic>

#include <nn/os/os_Event.h>
#include <nn/os/os_MessageQueue.h>
#include <nn/util/util_BytePtr.h>

#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_WavOutFileStream.h>
#include <nn/atk/fnd/os/atkfnd_Thread.h>

namespace nn::atk {

class DeviceOutRecorder : public detail::fnd::Thread::Handler {
    NN_NO_COPY(DeviceOutRecorder);

public:
    static const uint32_t RecordingBufferSize = 0x5a000;
    static const uint32_t DefaultWriteBlockPerSamples = 0x10000;
    static const uint32_t RequiredThreadStackSize = 0x10000;

    enum State {
        State_NotInitialized,
        State_Initialized,
        State_Recording,
        State_Recorded,
    };

    enum Message {
        Message_Prepare,
        Message_WriteSamples,
        Message_RequestStop,
        Message_Exit,
    };

    class InitializationOptions {
    public:
        InitializationOptions() = default;

        uint32_t GetPrioritiy() const { return m_Priority; }
        void SetPriority(uint32_t value) { m_Priority = value; }

        int GetIdealCoreNumber() const { return m_IdealCoreNumber; }
        void SetIdealCoreNumber(int value) { m_IdealCoreNumber = value; }

    private:
        uint32_t m_Priority;
        int m_IdealCoreNumber;
    };
    static_assert(sizeof(InitializationOptions) == 0x8);

    class RecordingOptions {
    public:
        RecordingOptions() = default;

        uint32_t GetChannels() const { return m_Channels; }
        void SetChannels(uint32_t value) { m_Channels = value; }

        bool IsLeadSilenceTrimmingEnabled() const { return m_IsLeadSilenceTrimmingEnabled; }
        void SetLeadSilenceTrimmingEnabled(bool value) { m_IsLeadSilenceTrimmingEnabled = value; }

        uint32_t GetMaxFrames() const { return m_MaxFrames; }
        void SetMaxFrames(uint32_t value) { m_MaxFrames = value; }

        uint32_t GetWriteBlockPerSamples() const { return m_WriteBlockPerSamples; }
        void SetWriteBlockPerSamples(uint32_t value) { m_WriteBlockPerSamples = value; }

    private:
        uint32_t m_Channels;
        bool m_IsLeadSilenceTrimmingEnabled;
        uint32_t m_MaxFrames;
        uint32_t m_WriteBlockPerSamples;
    };
    static_assert(sizeof(RecordingOptions) == 0x10);

    class RecorderBuffer {
        struct WriteState {
            uint32_t channelIndex;
            uint32_t writtenSampleCount;
        };
        static_assert(sizeof(WriteState) == 0x8);

        explicit RecorderBuffer(const char* deviceName);

        void Initialize(int16_t* sampleBuffer, uint32_t maxSamples);
        void Finalize();

        uint32_t Push(const int16_t* sampleBuffer, uint32_t samples);
        uint32_t Pop(uint32_t samples);
        int16_t* Peek();

        void SetReadBlockSamples(uint32_t value);
        void Clear();

        uint32_t GetReadableCount() const;
        uint32_t GetWritableCount() const;
        uint32_t GetContiguousReadableCount() const;
        const char* GetDeviceName() const;

        void UpdateMaxSamples();

    private:
        void Skip(uint32_t samples);
        void Write(const int16_t* sampleBuffer, uint32_t samples);
        uint32_t IncrementPosition(uint32_t position, uint32_t length) const;

        int16_t* m_SampleBuffer{nullptr};
        uint32_t m_MaxBufferSamples{0};
        uint32_t m_MaxSamples{0};
        std::atomic_uint m_ValidSamples{0};
        uint32_t m_ReadPosition{0};
        uint32_t m_WritePosition{0};
        uint32_t m_ReadBlockSamples{1};
        WriteState m_WriteState;
        const char* m_DeviceName;
    };
    static_assert(sizeof(RecorderBuffer) == 0x30);

protected:
    explicit DeviceOutRecorder(const char* deviceName);

public:
    ~DeviceOutRecorder() override;

    bool Initialize(void* recordingBuffer, size_t recordingBufferSize, void* pThreadStack,
                    size_t threadStackSize);
    bool Initialize(void* recordingBuffer, size_t recordingBufferSize, void* pThreadStack,
                    size_t threadStackSize, const InitializationOptions& options);

    void Finalize();

    size_t GetRequiredMemorySizeForRecording();

    bool Start(detail::fnd::FileStream& fileStream);
    bool Start(detail::fnd::FileStream& fileStream, const RecordingOptions& options);

    void Stop(bool isBlocking);

    bool IsInitialized() const;

    State GetState() const;

    bool IsLeadSilenceTrimming() const;

    uint32_t GetRecordingChannels() const;

    void RecordSamples(const int16_t* sampleBuffer, uint32_t samples);

protected:
    OutputMode GetOutputMode() const;

    virtual uint32_t GetMaxFrameLength() const = 0;
    virtual uint32_t GetSamplesPerSec() const = 0;
    virtual uint32_t GetValidChannels() const = 0;

    virtual void OnStart();
    virtual void OnStop();
    virtual uint32_t OnProcessSamples(int16_t* sampleBuffer, uint32_t samples);

    int16_t ResolveSampleEndian(int16_t sample);

    uint32_t Run(void* param) override;

private:
    int32_t GetReadBlockSamples(uint32_t channels) const;
    uint32_t GetLeadSilenceSamples(const int16_t* sampleBuffer, uint32_t samples,
                                   uint32_t channels) const;
    uint32_t GetWritableSamples(uint32_t samples) const;
    bool IsNoMoreSamples() const;

    bool StartThread(uint32_t, int32_t);
    void StopThread();

    int32_t Prepare();
    bool SendMessage(Message message);
    bool PostMessage(Message message);

    int32_t OnPrepare();
    void OnRequestStop();
    void OnExit();
    bool OnWriteSamples(bool isForceWriteMode);

    volatile State m_State{State_NotInitialized};
    uint32_t m_Channels{0};
    OutputMode m_OutputMode{OutputMode_Stereo};
    bool m_IsLeadSilenceTrimming{false};
    uint32_t m_MaxSamples{0};
    uint32_t m_WrittenSamples{0};
    detail::fnd::Thread m_Thread;
    void* m_ThreadStack;
    os::MessageQueue m_MessageQueue;
    std::uintptr_t m_Message;
    int32_t m_MessageResult;
    os::Event m_MessageDoneEvent;
    detail::fnd::FileStream* m_Stream;
    detail::WavOutFileStream m_WavOutStream;
    RecorderBuffer m_RecordingBuffer;
    util::BytePtr m_WorkBuffer;
    uint32_t m_WriteBlockPerSamples;
};
static_assert(sizeof(DeviceOutRecorder) == 0x310);

}  // namespace nn::atk
