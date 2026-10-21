#include <nn/atk/atk_VoiceCommand.h>

namespace nn::atk::detail {

LowLevelVoiceCommand& LowLevelVoiceCommand::GetInstance() {
    static LowLevelVoiceCommand instance;
    return instance;
}

}  // namespace nn::atk::detail
