#pragma once

#include "IFrameProcessor.h"          // InferenceDetection
#include "FramePreprocessor.h"        // LetterboxParams

#include <QVector>

#include <cstdint>
#include <vector>

namespace Labs {

class DetectionPostprocessor {
public:
    // Decodes a YOLOv8 detection-head output of shape [1, 4 + nc, N]
    // (channel-major; Ultralytics ONNX export default) into normalized
    // source-frame detections.
    //
    // Pipeline: per-anchor argmax over class channels → confidence
    // threshold → class-aware greedy NMS → inverse letterbox → divide
    // by source frame size.
    //
    // Returns false if the shape doesn't match the expected layout (so
    // the plugin can log + bail instead of pretending it produced
    // results); on shape mismatch `out` is left empty.
    static bool parseYolov8(const float* output,
                              const std::vector<int64_t>& outputShape,
                              int modelW, int modelH,
                              int srcW,   int srcH,
                              const LetterboxParams& lb,
                              float confThreshold,
                              float nmsIou,
                              QVector<InferenceDetection>* out);
};

} // namespace Labs
