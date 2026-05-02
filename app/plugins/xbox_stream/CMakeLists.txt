set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/third-party/ffmpeg" CACHE PATH "FFmpeg prebuilt root")

add_labs_plugin(XboxStreamPlugin
    XboxStreamPlugin.h
    XboxStreamPlugin.cpp
    XboxStreamSession.h
    XboxStreamSession.cpp
    XboxSignInDialog.h
    XboxSignInDialog.cpp
    XboxConsolePickerDialog.h
    XboxConsolePickerDialog.cpp
)

target_compile_definitions(XboxStreamPlugin PRIVATE
    XBOX_SIDECAR_DIR_DEV="${CMAKE_SOURCE_DIR}/sidecars/xbox-stream"
)

target_include_directories(XboxStreamPlugin PRIVATE
    "${FFMPEG_DIR}/include"
)

target_link_directories(XboxStreamPlugin PRIVATE
    "${FFMPEG_DIR}/lib"
)

target_link_libraries(XboxStreamPlugin PRIVATE
    Qt6::Network
    "${FFMPEG_DIR}/lib/libavcodec.dll.a"
    "${FFMPEG_DIR}/lib/libavutil.dll.a"
    "${FFMPEG_DIR}/lib/libswscale.dll.a"
)

# Run `npm install` once when package.json changes. The plugin's runtime
# locator prefers the source tree, so we don't mirror node_modules into the
# build output — that's the installer's job for production.

set(XBOX_SIDECAR_SRC   "${CMAKE_SOURCE_DIR}/sidecars/xbox-stream")
set(XBOX_INSTALL_STAMP "${CMAKE_BINARY_DIR}/xbox-stream-sidecar-install.stamp")

find_program(NPM_EXECUTABLE NAMES npm.cmd npm)
if(NPM_EXECUTABLE)
    add_custom_command(
        OUTPUT "${XBOX_INSTALL_STAMP}"
        COMMAND ${NPM_EXECUTABLE} install --no-audit --no-fund
        COMMAND ${CMAKE_COMMAND} -E touch "${XBOX_INSTALL_STAMP}"
        WORKING_DIRECTORY "${XBOX_SIDECAR_SRC}"
        DEPENDS "${XBOX_SIDECAR_SRC}/package.json"
        COMMENT "Installing Xbox Stream sidecar npm dependencies"
        VERBATIM
    )
    add_custom_target(xbox_stream_sidecar_deps DEPENDS "${XBOX_INSTALL_STAMP}")
    add_dependencies(XboxStreamPlugin xbox_stream_sidecar_deps)
else()
    message(WARNING
        "npm not found on PATH — run `npm install` manually in ${XBOX_SIDECAR_SRC} "
        "before launching.")
endif()
