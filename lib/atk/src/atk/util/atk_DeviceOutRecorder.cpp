#include <nn/atk/atk_DeviceOutRecorder.h>

#include <nn/atk/atk_SoundSystem.h>

namespace nn::atk {

DeviceOutRecorder::RecorderBuffer::RecorderBuffer(const char* deviceName) {
    m_DeviceName = deviceName == nullptr ? "" : deviceName;
}

void DeviceOutRecorder::RecorderBuffer::Initialize(int16_t* sampleBuffer, uint32_t maxSamples) {
    m_SampleBuffer = sampleBuffer;
    m_MaxBufferSamples = maxSamples;
    m_WriteState.channelIndex = 0;
    m_WriteState.writtenSampleCount = 0;
    UpdateMaxSamples();
    Clear();
}

void DeviceOutRecorder::RecorderBuffer::Finalize() {
    m_SampleBuffer = nullptr;
    m_MaxSamples = 0;
    m_ReadBlockSamples = 1;
}

uint32_t DeviceOutRecorder::RecorderBuffer::Push(const int16_t* sampleBuffer, uint32_t samples) {
    if (samples == 0)
        return 0;

    uint32_t copyCount{GetWritableCount()};
    if (samples <= copyCount)
        copyCount = samples;

    if (copyCount == 0)
        return 0;

    Write(sampleBuffer, copyCount);
    return copyCount;
}

uint32_t DeviceOutRecorder::RecorderBuffer::Pop(uint32_t samples) {
    if (samples == 0)
        return 0;

    uint32_t skipCount{GetContiguousReadableCount()};
    if (skipCount >= samples)
        skipCount = samples;

    Skip(skipCount);
    if (skipCount == samples)
        return samples;

    samples -= skipCount;

    uint32_t restCount{GetContiguousReadableCount()};
    if (restCount >= samples)
        restCount = samples;

    Skip(restCount);
    return restCount + skipCount;
}

int16_t* DeviceOutRecorder::RecorderBuffer::Peek() {
    if (GetReadableCount() == 0)
        return nullptr;

    return m_SampleBuffer + m_ReadPosition;
}

void DeviceOutRecorder::RecorderBuffer::SetReadBlockSamples(uint32_t value) {
    m_ReadBlockSamples = value <= 1 ? 1 : value;
    UpdateMaxSamples();
}

void DeviceOutRecorder::RecorderBuffer::Clear() {
    m_ValidSamples = 0;
    m_ReadPosition = 0;
    m_WritePosition = 0;
}

void DeviceOutRecorder::RecorderBuffer::UpdateMaxSamples() {
    if (m_ReadBlockSamples != 0)
        m_MaxSamples = (m_MaxBufferSamples / m_ReadBlockSamples) * m_ReadBlockSamples;
    else
        m_MaxSamples = m_MaxBufferSamples;
}

inline void DeviceOutRecorder::RecorderBuffer::Skip(uint32_t samples) {
    m_ReadPosition = IncrementPosition(m_ReadPosition + samples, m_MaxSamples);
    m_ValidSamples -= samples;
}

inline void DeviceOutRecorder::RecorderBuffer::Write(const int16_t* sampleBuffer,
                                                     uint32_t samples) {
    uint32_t channels{
        static_cast<uint32_t>(detail::driver::HardwareManager::GetInstance().GetChannelCountMax()),
    };
    uint32_t sampleCountPerChannel{
        static_cast<uint32_t>(
            detail::driver::HardwareManager::GetInstance().GetRendererSampleCount()),
    };

    for (uint32_t i{0}; i < samples; ++i) {
        uint32_t writeTo{
            IncrementPosition(m_WritePosition + m_WriteState.channelIndex +
                                  m_WriteState.writtenSampleCount * channels,
                              m_MaxSamples),
        };
        m_SampleBuffer[writeTo] = sampleBuffer[i];
        m_WriteState.writtenSampleCount += 1;

        if (m_WriteState.writtenSampleCount >= sampleCountPerChannel) {
            m_WriteState.channelIndex += 1;
            m_WriteState.writtenSampleCount = 0;
            if (m_WriteState.channelIndex >= channels) {
                m_WriteState.channelIndex = 0;
                uint32_t readSamples{m_WritePosition + sampleCountPerChannel * channels};
                m_WritePosition = IncrementPosition(readSamples, m_MaxSamples);
                m_ValidSamples += sampleCountPerChannel * channels;
            }
        }
    }
}

inline uint32_t DeviceOutRecorder::RecorderBuffer::GetWritableCount() const {
    uint32_t channels{
        static_cast<uint32_t>(detail::driver::HardwareManager::GetInstance().GetChannelCountMax()),
    };
    uint32_t sampleCountPerChannel{
        static_cast<uint32_t>(
            detail::driver::HardwareManager::GetInstance().GetRendererSampleCount()),
    };
    uint32_t unWritableSampleCount{
        static_cast<uint32_t>(m_ValidSamples) + sampleCountPerChannel * channels,
    };

    if (m_MaxSamples < unWritableSampleCount)
        return 0;

    return m_MaxSamples - unWritableSampleCount;
}

inline uint32_t DeviceOutRecorder::RecorderBuffer::GetContiguousReadableCount() const {
    volatile uint32_t validSamples{m_ValidSamples};
    uint32_t samples;

    if (m_ReadPosition > m_WritePosition)
        samples = m_MaxSamples - m_ReadPosition;
    else
        samples = validSamples;

    if (m_ReadBlockSamples != 0)
        samples = (samples / m_ReadBlockSamples) * m_ReadBlockSamples;

    return samples;
}

inline uint32_t DeviceOutRecorder::RecorderBuffer::IncrementPosition(uint32_t position,
                                                                     uint32_t length) const {
    uint32_t result = position;

    if (length <= position)
        result = position - length;

    return result;
}

DeviceOutRecorder::DeviceOutRecorder(const char* deviceName) : m_RecordingBuffer{deviceName} {}

DeviceOutRecorder::~DeviceOutRecorder() {
    Finalize();
}

void DeviceOutRecorder::Finalize() {
    if (!IsInitialized())
        return;

    detail::driver::HardwareManager::GetInstance().UnregisterRecorder(this);

    if (m_State == State_Recording || m_State == State_Recorded)
        Stop(true);

    StopThread();

    m_WorkBuffer.Reset(nullptr);
    m_RecordingBuffer.Clear();
    m_RecordingBuffer.Finalize();
    m_State = State_NotInitialized;
    m_ThreadStack = nullptr;
}

bool DeviceOutRecorder::Initialize(void* recordingBuffer, size_t recordingBufferSize,
                                   void* pThreadStack, size_t threadStackSize) {
    InitializationOptions param;
    return Initialize(recordingBuffer, recordingBufferSize, pThreadStack, threadStackSize, param);
}

bool DeviceOutRecorder::Initialize(void* recordingBuffer, size_t recordingBufferSize,
                                   void* pThreadStack, [[maybe_unused]] size_t threadStackSize,
                                   const InitializationOptions& options) {
    if (IsInitialized())
        return false;

    const uint32_t maxFrameLength{GetMaxFrameLength()};
    const size_t bufferLength{recordingBufferSize / maxFrameLength};
    m_RecordingBuffer.Initialize(static_cast<int16_t*>(recordingBuffer),
                                 (bufferLength * maxFrameLength) / 2);

    m_ThreadStack = pThreadStack;

    if (!StartThread(options.GetPrioritiy(), options.GetIdealCoreNumber()))
        return false;

    if (!detail::driver::HardwareManager::GetInstance().RegisterRecorder(this))
        return false;

    m_State = State_Initialized;
    return true;
}

bool DeviceOutRecorder::StartThread(uint32_t priority, int32_t idealCoreNumber) {
    uintptr_t message;
    while (m_MessageQueue.TryReceive(&message)) {
    }

    m_MessageDoneEvent.Clear();

    detail::fnd::Thread::RunArgs args;
    args.name = "nn::atk::DeviceOutRecorder";
    args.stack = m_ThreadStack;
    args.stackSize = RequiredThreadStackSize;
    args.idealCoreNumber = idealCoreNumber;
    args.priority = priority;
    args.param = nullptr;
    args.handler = this;

    return m_Thread.Run(args);
}

size_t DeviceOutRecorder::GetRequiredMemorySizeForRecording() {
    return RecordingBufferSize;
}

void DeviceOutRecorder::Stop(bool isBlocking) {
    if (!IsInitialized())
        return;

    State state{static_cast<State>(m_State | State_Initialized)};
    if (state == State_Recorded)
        OnStop();

    if (isBlocking)
        m_MessageDoneEvent.Clear();

    uintptr_t message;
    while (m_MessageQueue.TryReceive(&message)) {
    }

    SendMessage(Message_RequestStop);

    if (isBlocking)
        m_MessageDoneEvent.Wait();

    m_Stream = nullptr;
}

void DeviceOutRecorder::StopThread() {
    if (!IsInitialized())
        return;

    uintptr_t message;
    while (m_MessageQueue.TryReceive(&message)) {
    }
    SendMessage(Message_Exit);

    m_Thread.WaitForExit();
    m_Thread.Release();
}

bool DeviceOutRecorder::Start(detail::fnd::FileStream& fileStream,
                              const RecordingOptions& options) {
    if (!IsInitialized())
        return false;

    if (m_State == State_Recording)
        return true;

    m_OutputMode = SoundSystem::GetOutputMode(OutputDevice_Main);
    m_Channels = GetValidChannels(options.GetChannels());
    m_Stream = &fileStream;
    m_IsLeadSilenceTrimming = options.IsLeadSilenceTrimmingEnabled();
    m_MaxSamples = options.GetMaxFrames() * m_Channels;
    m_WriteBlockPerSamples = options.GetWriteBlockPerSamples();
    m_WrittenSamples = 0;

    m_RecordingBuffer.SetReadBlockSamples(GetReadBlockSamples(m_Channels));
    m_RecordingBuffer.Clear();

    if (Prepare() < 0) {
        Stop(false);
        return false;
    }

    OnStart();
    return true;
}

uint32_t DeviceOutRecorder::GetReadBlockSamples(uint32_t channels) const {
    return channels * sizeof(int16_t);
}

int32_t DeviceOutRecorder::Prepare() {
    if (!IsInitialized())
        return -1;

    m_MessageDoneEvent.Clear();
    m_MessageQueue.Send(Message_Prepare);
    m_MessageDoneEvent.Wait();

    return m_MessageResult;
}

bool DeviceOutRecorder::SendMessage(Message message) {
    if (!IsInitialized())
        return false;

    m_MessageQueue.Send(message);
    return true;
}

void DeviceOutRecorder::RecordSamples(const int16_t* sampleBuffer, uint32_t samples) {
    if (!IsInitialized())
        return;

    if (m_State == State_Recording) {
        m_RecordingBuffer.Push(sampleBuffer, samples);
        PostMessage(Message_WriteSamples);
    }
}

bool DeviceOutRecorder::PostMessage(Message message) {
    if (!IsInitialized())
        return false;

    return m_MessageQueue.TrySend(message);
}

uint32_t DeviceOutRecorder::Run([[maybe_unused]] void* param) {
    bool shouldExit{false};

    while (!shouldExit) {
        uintptr_t msgBuf;
        m_MessageQueue.Receive(&msgBuf);

        switch (msgBuf) {
        case Message_Prepare:
            m_MessageResult = OnPrepare();
            if (m_MessageResult == 0)
                m_State = State_Recording;
            m_MessageDoneEvent.Signal();
            break;
        case Message_WriteSamples:
            if (OnWriteSamples(false) == 0)
                m_State = State_Recorded;
            break;
        case Message_RequestStop:
            OnWriteSamples(true);
            OnRequestStop();
            m_State = State_Initialized;
            m_MessageDoneEvent.Signal();
            break;
        case Message_Exit:
            shouldExit = true;
            break;
        default:
            break;
        }
    }

    return 0;
}

int32_t DeviceOutRecorder::OnPrepare() {
    if (m_Stream == nullptr)
        return -1;

    if (!m_Stream->IsOpened())
        return -1;

    if (!m_WavOutStream.Open(*m_Stream, m_Channels, GetSamplesPerSec()) ||
        !m_WavOutStream.IsAvailable())
        return -1;

    return 0;
}

// NON_MATCHING: requires DeviceOutRecorder::GetLeadSilenceSamples
bool DeviceOutRecorder::OnWriteSamples(bool isForceWriteMode) {
    if (!IsInitialized())
        return false;

    while (m_State == State_Recording) {
        uint32_t samples{m_RecordingBuffer.GetContiguousReadableCount()};
        if (m_RecordingBuffer.GetReadableCount() < m_WriteBlockPerSamples && !isForceWriteMode)
            break;

        int16_t* sampleBuffer{m_RecordingBuffer.Peek()};
        if (sampleBuffer == nullptr)
            break;

        if (m_IsLeadSilenceTrimming) {
            uint32_t silenceSamples{GetLeadSilenceSamples(sampleBuffer, samples, m_Channels)};
            sampleBuffer += silenceSamples;
        }

        if (!IsNoMoreSamples()) {
            if (!m_WavOutStream.IsAvailable())
                return false;

            if (!m_Stream->IsOpened())
                return false;

            uint32_t writeLength{GetWritableSamples(OnProcessSamples(sampleBuffer, samples)) * 2};
            size_t result{m_WavOutStream.Write(sampleBuffer, writeLength)};

            m_WrittenSamples += result;
        }

        m_RecordingBuffer.Pop(samples);
    }

    return !IsNoMoreSamples();
}

void DeviceOutRecorder::OnRequestStop() {
    m_WavOutStream.Close();
}

void DeviceOutRecorder::OnExit() {}

// NON_MATCHING
uint32_t DeviceOutRecorder::GetLeadSilenceSamples(const int16_t* sampleBuffer, uint32_t samples,
                                                  uint32_t channels) const {
    if (samples == 0)
        return 0;

    if (channels == 0)
        while (true) {
        }

    uint32_t result{0};

    for (uint32_t frameLeadIndex{0}; result < samples; result += channels, ++frameLeadIndex) {
        bool isSilentFrame{true};
        for (uint32_t sampleOffset{result}; sampleOffset < channels; ++sampleOffset) {
            isSilentFrame = sampleBuffer[sampleOffset] != 0;
            if (isSilentFrame)
                return result;
        }
    }

    return result;
}

uint32_t DeviceOutRecorder::GetWritableSamples(uint32_t samples) const {
    if (m_MaxSamples == 0)
        return samples;

    if (IsNoMoreSamples())
        return 0;

    if (m_MaxSamples - m_WrittenSamples < samples)
        return m_MaxSamples - m_WrittenSamples;

    return samples;
}

bool DeviceOutRecorder::IsNoMoreSamples() const {
    if (m_MaxSamples == 0)
        return false;

    return m_MaxSamples <= m_WrittenSamples;
}

void DeviceOutRecorder::OnStart() {}

}  // namespace nn::atk
