#pragma once

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundFileReader.h>

namespace nn::atk::detail::driver {

class AdvancedWaveSoundPlayer : public BasicSoundPlayer, public SoundThread::PlayerCallback {
public:
    struct PrepareParameter {
        SoundArchive::AdvancedWaveSoundInfo advancedWaveSoundInfo;
        UpdateType updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        int32_t subMixIndex;
#endif
        const void* pAwsdFile;
        const void* pWarcFile;
    };
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static_assert(sizeof(PrepareParameter) == 0x20);
#else
    static_assert(sizeof(PrepareParameter) == 0x18);
#endif

    AdvancedWaveSoundPlayer();
    ~AdvancedWaveSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void Start() override;
    void Stop() override;
    void Pause(bool isPauseEnabled) override;

    void Prepare(const PrepareParameter& parameter);

    bool IsPrepared() const { return m_IsPrepared; }

    UpdateType GetUpdateType() const { return m_UpdateType; }

protected:
    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;

private:
    struct ClipParam {
        bool isPlayed;
        Channel* pChannel;
    };
    static_assert(sizeof(ClipParam) == 0x10);

    struct TrackParam {
        static const int32_t ClipParamCountMax{10};

        bool isPlayed;
        ClipParam clipParam[ClipParamCountMax];
    };
    static_assert(sizeof(TrackParam) == 0xa8);

    struct TrackParamSet {
        static const int32_t TrackParamCountMax{4};

        bool isPlayed;
        TrackParam trackParam[TrackParamCountMax];
    };
    static_assert(sizeof(TrackParamSet) == 0x2a8);

    void OnShutdownSoundThread() override;

    void Update();
    void SetupPlayer();
    void TearDownPlayer();

    bool SetupTracks();
    bool UpdateTracks();
    void ReleaseTracks();

    bool StartClip(ClipParam* pClipParam, SoundArchive::AdvancedWaveSoundInfo* pWaveSoundClipInfo);
    bool UpdateClip(ClipParam* pClipParam, SoundArchive::AdvancedWaveSoundInfo* pWaveSoundClipInfo);
    void ReleaseClip(ClipParam* pClipParam);
    void StopClip(ClipParam* pClipParam);

    void InitializeTrackParams();

    SoundArchive::AdvancedWaveSoundInfo m_AdvancedWaveSoundInfo;
    AdvancedWaveSoundTrackInfoSet m_AdvancedWaveSoundTrackInfoSet;
    TrackParamSet m_TrackParamSet;
    const void* m_pAwsdFile{};
    const void* m_pWarcFile{};
    UpdateType m_UpdateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    int32_t m_SubMixIndex;
#endif
    uint32_t m_CurrentTime{0};
    bool m_IsPrepared{false};
    bool m_IsInitialized{false};
    bool m_IsRegisterPlayerCallback{false};
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(AdvancedWaveSoundPlayer) == 0x768);
#else
static_assert(sizeof(AdvancedWaveSoundPlayer) == 0x778);
#endif

}  // namespace nn::atk::detail::driver
