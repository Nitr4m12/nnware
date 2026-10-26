#include <nn/atk/atk_SoundArchivePlayer.h>

namespace {

const int32_t MemAlignSize{64};
const size_t FileLoadAlignSize{256};

size_t GetRequiredSoundArchivePlayerMemSizeWithoutStreamSoundInstance(
    nn::atk::SoundArchive* arc, size_t userParamSizePerSound, int32_t addonSoundArchiveCount);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
nn::atk::OutputReceiver* GetOutputReceiver(const nn::atk::detail::StartInfoReader& reader,
                                           nn::atk::OutputReceiver* pDefaultOutputReceiver);
#endif

const nn::atk::SoundArchive*
GetTargetSoundArchive(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                      const char* soundArchiveName);
const nn::atk::SoundDataManager*
GetTargetSoundDataManager(const nn::atk::detail::SoundArchiveManager& soundArchiveManager,
                          const char* soundArchiveName);

size_t GetSoundPlayerInstanceSize(const nn::atk::detail::SoundInstanceConfig& config);
void SetupSoundInstanceConfigForSoundPlayer(const nn::atk::detail::SoundInstanceConfig& config);

}  // anonymous namespace

namespace nn::atk {

SoundArchivePlayer::SoundArchivePlayer() = default;

}  // namespace nn::atk
