#include <nn/atk/atk_DriverCommand.h>

namespace nn::atk::detail {

DriverCommand& DriverCommand::GetInstance() {
    static DriverCommand instance;
    return instance;
}

DriverCommand::DriverCommand() = default;

}  // namespace nn::atk::detail
