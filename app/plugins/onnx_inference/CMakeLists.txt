# OnnxInferencePlugin — self-owned ONNX Runtime IFrameProcessor (YOLOv8).
#
# Requires ONNX Runtime for Windows x64. Drop the official release into:
#
#   app/third-party/onnxruntime/
#     include/onnxruntime_cxx_api.h
#     lib/onnxruntime.lib
#     lib/onnxruntime.dll       (and any sibling provider DLLs)
#
# Source: https://github.com/microsoft/onnxruntime/releases
# CPU build (onnxruntime-win-x64-1.20.x.zip) is enough; GPU build
# (onnxruntime-win-x64-gpu-*.zip) also works and unlocks the optional
# CUDA execution provider — its extra provider DLLs are picked up by
# the file(GLOB) below.
#
# When the SDK is missing this plugin self-skips with a STATUS message
# so the rest of the engine still builds.

set(ORT_ROOT    "${CMAKE_SOURCE_DIR}/third-party/onnxruntime")
set(ORT_INCLUDE "${ORT_ROOT}/include")
set(ORT_LIB     "${ORT_ROOT}/lib/onnxruntime.lib")

if(NOT EXISTS "${ORT_INCLUDE}/onnxruntime_cxx_api.h" OR NOT EXISTS "${ORT_LIB}")
    message(STATUS
        "OnnxInferencePlugin: ONNX Runtime not found at ${ORT_ROOT} — plugin skipped. "
        "Drop a Windows x64 release zip's contents there to enable.")
    return()
endif()

add_labs_plugin(OnnxInferencePlugin
    OnnxInferencePlugin.h
    OnnxInferencePlugin.cpp
    OnnxSession.h
    OnnxSession.cpp
    FramePreprocessor.h
    FramePreprocessor.cpp
    DetectionPostprocessor.h
    DetectionPostprocessor.cpp
)

target_include_directories(OnnxInferencePlugin PRIVATE "${ORT_INCLUDE}")
target_link_libraries(OnnxInferencePlugin PRIVATE "${ORT_LIB}")

# Stage onnxruntime.dll (and any sibling provider DLLs in the release
# lib/ folder, e.g. CUDA / DirectML for the GPU build) next to the main
# exe so the plugin DLL resolves them at load time.
file(GLOB ORT_RUNTIME_DLLS "${ORT_ROOT}/lib/*.dll")
foreach(_dll ${ORT_RUNTIME_DLLS})
    add_custom_command(TARGET OnnxInferencePlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_dll}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endforeach()
