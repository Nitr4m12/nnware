#include <nn/atk/atk_Adpcm.h>

#include <climits>

namespace nn::atk::detail {

void DecodeDspAdpcm(position_t playPosition, AdpcmContext& context, const AdpcmParam& param,
                    const void* adpcmData, size_t decodeSamples, int16_t* dest) {
    position_t frame = playPosition / 14;
    position_t frameFrac = playPosition - frame * 14;
    const uint8_t* frameBegin = reinterpret_cast<const uint8_t*>(adpcmData) + (frame * 8);

    int32_t pred = context.audioAdpcmContext.predScale >> 4;
    int32_t scale = context.audioAdpcmContext.predScale & 0xF;

    for (uint32_t i{0}; i < decodeSamples; ++i) {
        if (frameFrac == 0) {
            const uint8_t pred_scale = *frameBegin;
            context.audioAdpcmContext.predScale = pred_scale;
            pred = pred_scale >> 4;
            scale = pred_scale & 0xF;
        }

        uint8_t code = frameBegin[frameFrac / 2 + 1];
        if (frameFrac & 1)
            code &= 0xF;
        else
            code >>= 4;

        int16_t nibble = code;
        nibble <<= 12;
        nibble >>= 1;

        int16_t a1 = static_cast<int16_t>(param.coefficients[pred * 2 + 0]);
        int16_t a2 = static_cast<int16_t>(param.coefficients[pred * 2 + 1]);
        int16_t gain = static_cast<int16_t>(1 << scale);

        int32_t val = a1 * context.audioAdpcmContext.history[0];
        val += a2 * context.audioAdpcmContext.history[1];
        val += gain * nibble;
        val >>= 10;
        val += 1;
        val >>= 1;

        if (val > SHRT_MAX)
            val = SHRT_MAX;
        else if (val < SHRT_MIN)
            val = SHRT_MIN;

        int16_t smp = static_cast<int16_t>(val);

        context.audioAdpcmContext.history[1] = context.audioAdpcmContext.history[0];
        context.audioAdpcmContext.history[0] = smp;

        *dest = smp;
        ++dest;

        ++frameFrac;
        if (frameFrac == 14) {
            frameBegin += 8;
            frameFrac = 0;
        }
    }
}

}  // namespace nn::atk::detail
