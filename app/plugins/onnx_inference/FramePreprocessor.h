#pragma once

#include "FrameTypes.h"

namespace Labs {

// Inverse-mapping data captured at preprocess time so postprocess can
// invert the letterbox to land detections in source-frame pixel space.
struct LetterboxParams {
    float scale = 1.0f;   // model-input pixel == src pixel * scale
    int   padX  = 0;      // model-input columns to skip on the left
    int   padY  = 0;      // model-input rows to skip on the top
};

class FramePreprocessor {
public:
    // Resamples a BGRA `src` frame into planar RGB float32 NCHW
    // [1, 3, modelH, modelW] in `out`, normalized to [0,1] and letterboxed
    // (aspect-preserving) with 114/255 gray pad. Returns the transform
    // params for postprocess inverse-mapping.
    //
    // `out` must point to a buffer of at least 3 * modelH * modelW floats.
    // Sampling is nearest-neighbor — for 1920×1080 → 640×640 the float
    // copy dominates over the sampler, so bilinear's 4× cost buys little
    // until small-object accuracy becomes the limiter.
    static LetterboxParams toRgbFloatNCHW(const Frame& src,
                                            int modelW, int modelH,
                                            float* out);
};

} // namespace Labs
