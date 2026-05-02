add_labs_plugin(CaptureCardPlugin
    CaptureCardPlugin.h
    CaptureCardPlugin.cpp
    MfCaptureSource.h
    MfCaptureSource.cpp
    CaptureDevicePickerDialog.h
    CaptureDevicePickerDialog.cpp
)

target_link_libraries(CaptureCardPlugin PRIVATE
    mf
    mfplat
    mfuuid
    mfreadwrite
    windowscodecs
    ole32
    shlwapi
)

target_compile_definitions(CaptureCardPlugin PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0A00
)
