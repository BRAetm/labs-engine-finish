add_labs_plugin(SmartCapturePlugin
    ProcessMonitor.h
    ProcessMonitor.cpp
    SmartCaptureProvider.h
    SmartCaptureProvider.cpp
    SmartCapturePlugin.h
    SmartCapturePlugin.cpp
)
target_link_libraries(SmartCapturePlugin PRIVATE d3d11 dxgi)
