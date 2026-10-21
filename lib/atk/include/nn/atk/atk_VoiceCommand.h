#pragma once

#include <nn/util.h>

#include <nn/atk/atk_CommandManager.h>
#include <nn/atk/atk_LowLevelVoice.h>

namespace nn::atk::detail {

enum VoiceCommandId {
    VoiceCommandId_Alloc,
    VoiceCommandId_Free,
    VoiceCommandId_Priority,
    VoiceCommandId_Play,
    VoiceCommandId_Pause,
    VoiceCommandId_Stop,
    VoiceCommandId_AppendWaveBuffer,
    VoiceCommandId_Param,
};

struct VoiceCommandAlloc : Command {
    uint32_t voiceId;
    uint32_t priority;
    void* userId;
};
static_assert(sizeof(VoiceCommandAlloc) == 0x28);

struct VoiceCommandFree : Command {
    uint32_t voiceId;
};
static_assert(sizeof(VoiceCommandFree) == 0x20);

struct VoiceCommandParam : Command {
    uint32_t voiceId;
    VoiceParam voiceParam;
};
static_assert(sizeof(VoiceCommandParam) == 0x98);

struct VoiceCommandPriority : Command {
    uint32_t voiceId;
    uint32_t priority;
};
static_assert(sizeof(VoiceCommandPriority) == 0x20);

struct VoiceCommandPlay : Command {
    uint32_t voiceId;
    SampleFormat sampleFormat;
    uint32_t sampleRate;
    AdpcmParam adpcmParam;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    int32_t subMixIndex;
#else
    OutputReceiver* pOutputReceiver;
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(VoiceCommandPlay) == 0x48);
#else
static_assert(sizeof(VoiceCommandPlay) == 0x50);
#endif

struct VoiceCommandPause : Command {
    uint32_t voiceId;
};
static_assert(sizeof(VoiceCommandPause) == 0x20);

struct VoiceCommandAppendWaveBuffer : Command {
    uint32_t voiceId;
    void* tag;
    const void* bufferAddress;
    size_t bufferSize;
    size_t sampleLength;
    position_t sampleOffset;
    bool adpcmContextEnable;
    AdpcmContext adpcmContext;
    bool loopFlag;
};
static_assert(sizeof(VoiceCommandAppendWaveBuffer) == 0x100);

class LowLevelVoiceCommand : public CommandManager {
public:
    __attribute__((noinline)) static LowLevelVoiceCommand& GetInstance();

    void Initialize(void* commandBuffer, size_t commandBufferSize, void* waveBuffer,
                    size_t waveBufferSize, int32_t waveBufferPacketCount);

    static size_t GetRequiredWaveBufferMemSize(int32_t waveBufferPacketCount);

private:
    struct WaveBufferPacket {
        AdpcmContext adpcmContext;
        WaveBuffer waveBuffer;
    };
    static_assert(sizeof(WaveBufferPacket) == 0x80);

    LowLevelVoiceCommand() = default;

    WaveBufferPacket* GetFreeWaveBuffer();

    static void ProcessCommandList(Command* commandList);

    static void LowLevelVoiceDisposeCallback(LowLevelVoice* voice, void* arg);

    WaveBufferPacket* m_pWaveBufferPacket;
    int32_t m_WaveBufferPacketCount;
};
static_assert(sizeof(LowLevelVoiceCommand) == 0x320);

class VoiceReplyCommand : public CommandManager {
public:
    static VoiceReplyCommand& GetInstance();

    void Initialize(void* commandBuffer, size_t commandBufferSize) {
        CommandManager::Initialize(commandBuffer, commandBufferSize, ProcessCommandList);
    }

private:
    VoiceReplyCommand() = default;

    static void ProcessCommandList(Command* commandList);
};
static_assert(sizeof(VoiceReplyCommand) == 0x310);

}  // namespace nn::atk::detail
