#include "OnnxSession.h"

#include <QDebug>

#include <array>

namespace Labs {

OnnxSession::OnnxSession()
    : m_env(ORT_LOGGING_LEVEL_WARNING, "Labs.OnnxInference")
{
}

OnnxSession::~OnnxSession() = default;

std::unique_ptr<OnnxSession> OnnxSession::create(const QString& modelPath,
                                                   Provider provider,
                                                   int requestedInputSize,
                                                   QString* errorOut)
{
    auto s = std::unique_ptr<OnnxSession>(new OnnxSession());
    s->m_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    s->m_options.SetIntraOpNumThreads(0);

    if (provider == Provider::Cuda) {
        // Only succeeds against the GPU build of ORT — the CPU build
        // throws here. We let it bubble up so the user sees a clear
        // error rather than silently falling back to CPU.
        try {
            OrtCUDAProviderOptions cudaOpts {};
            s->m_options.AppendExecutionProvider_CUDA(cudaOpts);
        } catch (const Ort::Exception& e) {
            if (errorOut) *errorOut = QStringLiteral("CUDA EP unavailable: %1").arg(e.what());
            return nullptr;
        }
    }

    try {
        const std::wstring path = modelPath.toStdWString();
        s->m_session = std::make_unique<Ort::Session>(s->m_env, path.c_str(), s->m_options);
    } catch (const Ort::Exception& e) {
        if (errorOut) *errorOut = QStringLiteral("Ort::Session: %1").arg(e.what());
        return nullptr;
    }

    const size_t nIn  = s->m_session->GetInputCount();
    const size_t nOut = s->m_session->GetOutputCount();
    if (nIn != 1 || nOut < 1) {
        if (errorOut) *errorOut = QStringLiteral("Model must have 1 input and >=1 output (got %1/%2)").arg(nIn).arg(nOut);
        return nullptr;
    }

    // Populate owned name vectors fully before grabbing c_str() views —
    // any push_back into m_inputNamesOwned after we've stashed pointers
    // could relocate the strings and invalidate them.
    for (size_t i = 0; i < nIn; ++i) {
        Ort::AllocatedStringPtr n = s->m_session->GetInputNameAllocated(i, s->m_alloc);
        s->m_inputNamesOwned.emplace_back(n.get());
    }
    for (size_t i = 0; i < nOut; ++i) {
        Ort::AllocatedStringPtr n = s->m_session->GetOutputNameAllocated(i, s->m_alloc);
        s->m_outputNamesOwned.emplace_back(n.get());
    }
    s->m_inputNamesOwned.shrink_to_fit();
    s->m_outputNamesOwned.shrink_to_fit();
    for (auto& n : s->m_inputNamesOwned)  s->m_inputNames.push_back(n.c_str());
    for (auto& n : s->m_outputNamesOwned) s->m_outputNames.push_back(n.c_str());

    auto inputInfo  = s->m_session->GetInputTypeInfo(0);
    auto inputShape = inputInfo.GetTensorTypeAndShapeInfo().GetShape();
    if (inputShape.size() != 4) {
        if (errorOut) *errorOut = QStringLiteral("Input must be 4D NCHW (got rank %1)").arg(inputShape.size());
        return nullptr;
    }
    int64_t h = inputShape[2];
    int64_t w = inputShape[3];
    if (h <= 0) h = requestedInputSize;
    if (w <= 0) w = requestedInputSize;
    s->m_inputH = static_cast<int>(h);
    s->m_inputW = static_cast<int>(w);
    return s;
}

bool OnnxSession::run(const float* input,
                       std::vector<float>* outputData,
                       std::vector<int64_t>* outputShape,
                       QString* errorOut)
{
    if (!m_session || !input || !outputData || !outputShape) return false;
    outputData->clear();
    outputShape->clear();

    try {
        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        const std::array<int64_t, 4> inputDims { 1, 3,
            static_cast<int64_t>(m_inputH), static_cast<int64_t>(m_inputW) };
        const size_t inputCount = static_cast<size_t>(3) * m_inputH * m_inputW;

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memInfo,
            const_cast<float*>(input), inputCount,
            inputDims.data(), inputDims.size());

        // Pull only the first output — extra outputs (segmentation
        // masks, embeddings, etc.) we don't consume in v1.
        const char* outName = m_outputNames.front();
        auto outputs = m_session->Run(
            Ort::RunOptions { nullptr },
            m_inputNames.data(), &inputTensor, 1,
            &outName, 1);

        Ort::Value& out0 = outputs.front();
        auto info = out0.GetTensorTypeAndShapeInfo();
        *outputShape = info.GetShape();

        const size_t n = info.GetElementCount();
        const float* src = out0.GetTensorData<float>();
        outputData->assign(src, src + n);
        return true;
    } catch (const Ort::Exception& e) {
        if (errorOut) *errorOut = QStringLiteral("Ort::Run: %1").arg(e.what());
        return false;
    }
}

} // namespace Labs
