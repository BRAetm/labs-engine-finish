#include "DetectionPostprocessor.h"

#include <algorithm>

namespace Labs {

namespace {

struct Box {
    float x1, y1, x2, y2;       // model-input pixel space, post-letterbox
    float score;
    int   classId;
};

inline float iou(const Box& a, const Box& b) {
    const float ix1 = std::max(a.x1, b.x1);
    const float iy1 = std::max(a.y1, b.y1);
    const float ix2 = std::min(a.x2, b.x2);
    const float iy2 = std::min(a.y2, b.y2);
    const float iw  = std::max(0.0f, ix2 - ix1);
    const float ih  = std::max(0.0f, iy2 - iy1);
    const float inter = iw * ih;
    const float ua    = (a.x2 - a.x1) * (a.y2 - a.y1)
                      + (b.x2 - b.x1) * (b.y2 - b.y1) - inter;
    return ua > 0.0f ? inter / ua : 0.0f;
}

} // namespace

bool DetectionPostprocessor::parseYolov8(const float* output,
                                           const std::vector<int64_t>& shape,
                                           int modelW, int modelH,
                                           int srcW,   int srcH,
                                           const LetterboxParams& lb,
                                           float confThreshold,
                                           float nmsIou,
                                           QVector<InferenceDetection>* out)
{
    out->clear();
    if (!output || shape.size() != 3 || shape[0] != 1) return false;
    if (srcW <= 0 || srcH <= 0 || modelW <= 0 || modelH <= 0) return false;

    const int channels = static_cast<int>(shape[1]);
    const int anchors  = static_cast<int>(shape[2]);
    if (channels < 5 || anchors <= 0) return false;

    const int numClasses = channels - 4;

    // YOLOv8 ONNX export is channel-major: stride along N (anchors) is 1,
    // stride along C (channels) is `anchors`. So output[c * anchors + a].
    auto val = [&](int c, int a) {
        return output[static_cast<int64_t>(c) * anchors + a];
    };

    std::vector<Box> candidates;
    candidates.reserve(64);

    for (int a = 0; a < anchors; ++a) {
        float bestScore = 0.0f;
        int   bestClass = -1;
        for (int c = 0; c < numClasses; ++c) {
            const float s = val(4 + c, a);
            if (s > bestScore) { bestScore = s; bestClass = c; }
        }
        if (bestScore < confThreshold || bestClass < 0) continue;

        const float cx = val(0, a);
        const float cy = val(1, a);
        const float w  = val(2, a);
        const float h  = val(3, a);

        Box b;
        b.x1 = cx - w * 0.5f;
        b.y1 = cy - h * 0.5f;
        b.x2 = cx + w * 0.5f;
        b.y2 = cy + h * 0.5f;
        b.score   = bestScore;
        b.classId = bestClass;
        candidates.push_back(b);
    }

    if (candidates.empty()) return true;

    std::sort(candidates.begin(), candidates.end(),
              [](const Box& a, const Box& b) { return a.score > b.score; });

    std::vector<Box> kept;
    kept.reserve(candidates.size());
    for (const Box& cand : candidates) {
        bool drop = false;
        for (const Box& k : kept) {
            if (k.classId != cand.classId) continue;
            if (iou(k, cand) > nmsIou) { drop = true; break; }
        }
        if (!drop) kept.push_back(cand);
    }

    const float invScale = lb.scale > 0.0f ? 1.0f / lb.scale : 1.0f;
    out->reserve(static_cast<int>(kept.size()));
    for (const Box& b : kept) {
        // Inverse letterbox: subtract pad in model space, divide by
        // scale to land in src pixels, clamp to src extents.
        const float sx1 = std::clamp((b.x1 - lb.padX) * invScale, 0.0f, static_cast<float>(srcW));
        const float sy1 = std::clamp((b.y1 - lb.padY) * invScale, 0.0f, static_cast<float>(srcH));
        const float sx2 = std::clamp((b.x2 - lb.padX) * invScale, 0.0f, static_cast<float>(srcW));
        const float sy2 = std::clamp((b.y2 - lb.padY) * invScale, 0.0f, static_cast<float>(srcH));

        InferenceDetection d;
        d.classId    = b.classId;
        d.confidence = b.score;
        d.x = sx1 / srcW;
        d.y = sy1 / srcH;
        d.w = (sx2 - sx1) / srcW;
        d.h = (sy2 - sy1) / srcH;
        out->push_back(d);
    }
    return true;
}

} // namespace Labs
