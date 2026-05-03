#pragma once

#include "LabsCore.h"
#include "FrameTypes.h"

#include <QtGlobal>
#include <QVector>

namespace Labs {

// Single AI detection produced by an IFrameProcessor. Bbox coords are
// normalized 0..1 against the source frame so consumers don't need to
// re-resolve when the capture resolution changes mid-stream.
struct InferenceDetection {
    int   classId    = -1;
    float confidence = 0.0f;
    float x          = 0.0f;
    float y          = 0.0f;
    float w          = 0.0f;
    float h          = 0.0f;
};

// One inference cycle's results. Sequence is monotonic per-processor;
// sinks dedupe on it because the underlying SHM may signal the same
// payload twice if our wait-loop wakes from a stale event.
struct InferenceResults {
    qint64                      frameTimestampUs = 0;
    int                         sessionId        = 0;
    quint32                     sequence         = 0;
    QVector<InferenceDetection> detections;
};

class LABSCORE_API IInferenceResultsSink {
public:
    virtual ~IInferenceResultsSink() = default;
    // Called from the processor's worker thread. Implementations must be
    // thread-safe or marshal to the UI thread themselves.
    virtual void onResults(const InferenceResults& results) = 0;
};

// Frame processor: a frame sink whose side effect is producing inference
// results asynchronously. pushFrame returns immediately; results land on
// the configured IInferenceResultsSink whenever the underlying engine
// completes a cycle.
class LABSCORE_API IFrameProcessor : public IFrameSink {
public:
    ~IFrameProcessor() override = default;

    virtual void setResultsSink(IInferenceResultsSink* sink) = 0;
    virtual bool start()                                     = 0;
    virtual void stop()                                      = 0;
    virtual bool isRunning() const                           = 0;
};

} // namespace Labs
