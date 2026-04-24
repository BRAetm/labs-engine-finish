set(VIGEM_ROOT "${CMAKE_SOURCE_DIR}/third-party/ViGEmClient")

add_labs_plugin(ViGEmPlugin
    ViGEmPlugin.h
    ViGEmPlugin.cpp
    ${VIGEM_ROOT}/src/ViGEmClient.cpp
)

target_include_directories(ViGEmPlugin PRIVATE
    ${VIGEM_ROOT}/include
    ${VIGEM_ROOT}/src
)

target_link_libraries(ViGEmPlugin PRIVATE
    setupapi
)
