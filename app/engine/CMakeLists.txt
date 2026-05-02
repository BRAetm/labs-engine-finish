add_executable(LabsEngine WIN32
    main.cpp
    LabsMainWindow.h
    LabsMainWindow.cpp
    LabsBackgroundWidget.h
    LabsLogoWidget.h
    LabsTheme.h
    LabsTheme.cpp
    LabsThemeDialog.h
    LabsThemeDialog.cpp
    ControllerMonitorWidget.h
    ControllerMonitorWidget.cpp
    Ps5Discovery.h
    Ps5Discovery.cpp
    # Marketplace lives in app/launcher/ but the engine compiles it too so
    # users get an in-engine catalog tab without having to bounce back to
    # the launcher. Same source, two binaries — both share the canonical
    # %LocalAppData%\Labs\scripts folder so installs from either surface
    # show up in both immediately.
    ${CMAKE_SOURCE_DIR}/launcher/MarketplaceWidget.h
    ${CMAKE_SOURCE_DIR}/launcher/MarketplaceWidget.cpp
    resources/labs.qrc
    resources/labs.rc
)

target_include_directories(LabsEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/launcher    # for MarketplaceWidget.h
)

target_link_libraries(LabsEngine PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Network
    LabsCore
)

set_target_properties(LabsEngine PROPERTIES
    OUTPUT_NAME "LabsEngine"
    WIN32_EXECUTABLE TRUE
)
