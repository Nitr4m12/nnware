#include <nn/atk/atk_SoundArchiveFilesHook.h>

namespace nn::atk::detail {

const char SoundArchiveFilesHook::ItemTypeWaveSound[] = "wsd";
const char SoundArchiveFilesHook::ItemTypeStreamSound[] = "stm";
const char SoundArchiveFilesHook::ItemTypeSequenceSound[] = "seq";

const char SoundArchiveFilesHook::FileTypeStreamBinary[] = "bxstm";
const char SoundArchiveFilesHook::FileTypeWaveSoundBinary[] = "bxwsd";
const char SoundArchiveFilesHook::FileTypeSequenceBinary[] = "bxseq";
const char SoundArchiveFilesHook::FileTypeBankBinary[] = "bxbnk";
const char SoundArchiveFilesHook::FileTypeWaveArchiveBinary[] = "bxwar";
const char SoundArchiveFilesHook::FileTypeStreamPrefetchBinary[] = "bxstp";

}  // namespace nn::atk::detail
