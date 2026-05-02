# LabsHub.exe — front door for Labs Engine + companion apps.
#
# Reuses MarketplaceWidget from app/engine/ by compiling the source directly
# into the launcher target. Same .cpp built twice (once for LabsEngine, once
# for LabsHub). AUTOMOC handles each target's own moc output.

add_executable(LabsHub WIN32
    main.cpp
    HubWindow.h
    HubWindow.cpp
    # MarketplaceWidget lives here now — moved out of app/engine/ so the engine
    # stops linking marketplace code at all. Hub owns discovery + install.
    MarketplaceWidget.h
    MarketplaceWidget.cpp
    # Real Labs branding — :/labs/labs.png, labs.ico. The qrc itself still
    # lives next to the engine because it also references engine-only SVG
    # icons; AUTORCC compiles it into both targets independently.
    ${CMAKE_SOURCE_DIR}/engine/resources/labs.qrc
)

target_include_directories(LabsHub PRIVATE
    ${CMAKE_SOURCE_DIR}/core      # for SettingsManager.h
)

target_link_libraries(LabsHub PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    LabsCore
)

set_target_properties(LabsHub PROPERTIES
    OUTPUT_NAME "LabsHub"
    WIN32_EXECUTABLE TRUE
)
