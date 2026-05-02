set(WV2_DIR "${CMAKE_SOURCE_DIR}/third-party/webview2" CACHE PATH "Microsoft Edge WebView2 SDK")

add_labs_plugin(XboxWebPlugin
    XboxWebPlugin.h
    XboxWebPlugin.cpp
    WebView2Widget.h
    WebView2Widget.cpp
)

target_include_directories(XboxWebPlugin PRIVATE
    "${WV2_DIR}/build/native/include"
)

target_link_directories(XboxWebPlugin PRIVATE
    "${WV2_DIR}/build/native/x64"
)

target_link_libraries(XboxWebPlugin PRIVATE
    WebView2Loader.dll.lib
    runtimeobject.lib
)

# Copy the loader DLL next to LabsEngine.exe.
add_custom_command(TARGET XboxWebPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WV2_DIR}/build/native/x64/WebView2Loader.dll"
        "$<TARGET_FILE_DIR:LabsEngine>/WebView2Loader.dll"
)
