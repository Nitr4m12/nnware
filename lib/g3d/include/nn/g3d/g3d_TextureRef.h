#pragma once

namespace nn::g3d {

struct TextureRef {};  // TODO

using TextureBindCallback = nn::g3d::TextureRef (*)(const char*, void*);

}  // namespace nn::g3d
