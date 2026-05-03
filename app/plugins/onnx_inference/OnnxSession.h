#pragma once

#include <QString>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

namespace Labs {

// Thin wrapper around Ort::Session that hides ORT types from the rest of
// the plugin. Owns the env, options, allocator, and cached I/O names.
// Single input / single output assumed (extra outputs are ignored —
// detection models often emit auxiliary tensors we don't consume).
class OnnxSession {
public:
    enum class Provider { Cpu, Cuda };

    // Loads `modelPath`, picks `provider`, and resolves the input dims.
    // If the model's spatial dims are dynamic (-1), `requestedInputSize`
    // is used for both H and W. On failure returns nullptr and writes a
    // human-readable reason into `errorOut`.
    static std::unique_ptr<OnnxSession> create(const QString& modelPath,
                                                 Provider provider,
                                                 int requestedInputSize,
                                                 QString* errorOut);
    ~OnnxSession();

    OnnxSession(const OnnxSession&)            = delete;
    OnnxSession& operator=(const OnnxSession&) = delete;

    int inputW() const { return m_inputW; }
    int inputH() const { return m_inputH; }

    // Runs the model with `input` shaped [1, 3, inputH(), inputW()].
    // Copies the first output tensor's float data into `outputData` and
    // its shape into `outputShape`. Returns false on Ort exception with
    // the message in `errorOut`.
    bool run(const float* input,
             std::vector<float>* outputData,
             std::vector<int64_t>* outputShape,
             QString* errorOut);

private:
    OnnxSession();

    Ort::Env                          m_env;
    Ort::SessionOptions               m_options;
    std::unique_ptr<Ort::Session>     m_session;
    Ort::AllocatorWithDefaultOptions  m_alloc;

    // Owned strings for I/O names — Ort returns allocator-managed
    // pointers we must keep alive for the lifetime of any Run() call
    // that references them. We cache c_str() views in m_inputNames /
    // m_outputNames once the owning vectors are populated.
    std::vector<std::string>          m_inputNamesOwned;
    std::vector<std::string>          m_outputNamesOwned;
    std::vector<const char*>          m_inputNames;
    std::vector<const char*>          m_outputNames;

    int                               m_inputW = 640;
    int                               m_inputH = 640;
};

} // namespace Labs
