#pragma once

#include <nn/util.h>

#include <nn/atk/atk_CommandManager.h>
#include <nn/atk/atk_LowLevelVoice.h>

namespace nn::atk::detail {

struct VoiceCommandPlay : Command {
    uint32_t voiceId;
    SampleFormat sampleFormat;
    uint32_t sampleRate;
    AdpcmParam adpcmParam;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
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

struct VoiceCommandAlloc : Command {
    uint32_t voiceId;
    uint32_t priority;
    void* userId;
};
static_assert(sizeof(VoiceCommandAlloc) == 0x28);

struct VoiceCommandAppendWaveBuffer : Command {
    uint32_t voiceId;
    void* tag;
    void* bufferAddress;
    size_t bufferSize;
    size_t sampleLength;
    position_t sampleOffset;
    bool adpcmContextEnable;
    AdpcmContext adpcmContext;
    bool loopFlag;
};
static_assert(sizeof(VoiceCommandAppendWaveBuffer) == 0x100);

class VoiceReplyCommand : CommandManager {
public:
    static void ProcessCommandList(Command* commandList);
};
static_assert(sizeof(VoiceReplyCommand) == 0x310);

class LowLevelVoiceCommand : CommandManager {
public:
    struct WaveBufferPacket {
        AdpcmContext adpcmContext;
        WaveBuffer waveBuffer;
    };
    static_assert(sizeof(WaveBufferPacket) == 0x80);

    void Initialize(void*, size_t, void*, size_t, int32_t);

    static void ProcessCommandList(Command* commandList);

    static size_t GetRequiredWaveBufferMemSize(int32_t);

    static void LowLevelVoiceDisposeCallback(LowLevelVoice*, void*);

    void* GetFreeWaveBuffer();

private:
    WaveBufferPacket* m_pWaveBufferPacket;
    int32_t m_WaveBufferPacketCount;
};
static_assert(sizeof(LowLevelVoiceCommand) == 0x320);

}  // namespace nn::atk::detail
