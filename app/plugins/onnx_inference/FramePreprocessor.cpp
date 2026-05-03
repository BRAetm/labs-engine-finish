#include "FramePreprocessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Labs {

namespace {
// 114/255 — Ultralytics YOLO default letterbox pad. Matches training.
constexpr float kPadValue = 114.0f / 255.0f;
} // namespace

LetterboxParams FramePreprocessor::toRgbFloatNCHW(const Frame& src,
                                                    int modelW, int modelH,
                                                    float* out)
{
    LetterboxParams lb;
    if (modelW <= 0 || modelH <= 0 || !out) return lb;

    const int plane = modelW * modelH;
    std::fill(out, out + 3 * plane, kPadValue);
    if (!src.isValid()) return lb;

    const float scale = std::min(static_cast<float>(modelW) / src.width,
                                  static_cast<float>(modelH) / src.height);
    const int newW = std::max(1, static_cast<int>(std::round(src.width  * scale)));
    const int newH = std::max(1, static_cast<int>(std::round(src.height * scale)));
    const int padX = (modelW - newW) / 2;
    const int padY = (modelH - newH) / 2;

    lb.scale = scale;
    lb.padX  = padX;
    lb.padY  = padY;

    float* rPlane = out;
    float* gPlane = out + plane;
    float* bPlane = out + 2 * plane;

    const auto* srcBytes = reinterpret_cast<const quint8*>(src.data.constData());
    const int   srcStride = src.stride;

    for (int dy = 0; dy < newH; ++dy) {
        const int sy = std::min(static_cast<int>(dy / scale), src.height - 1);
        const auto* srcRow = srcBytes + sy * srcStride;

        const int outY = dy + padY;
        float* rRow = rPlane + outY * modelW + padX;
        float* gRow = gPlane + outY * modelW + padX;
        float* bRow = bPlane + outY * modelW + padX;

        for (int dx = 0; dx < newW; ++dx) {
            const int sx = std::min(static_cast<int>(dx / scale), src.width - 1);
            const auto* px = srcRow + sx * 4;       // BGRA
            bRow[dx] = px[0] / 255.0f;
            gRow[dx] = px[1] / 255.0f;
            rRow[dx] = px[2] / 255.0f;
        }
    }
    return lb;
}

} // namespace Labs
