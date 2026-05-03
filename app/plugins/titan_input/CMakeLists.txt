add_labs_plugin(TitanInputPlugin
    TitanInputPlugin.h
    TitanInputPlugin.cpp
)

# Same Win32 deps as DualSenseInputPlugin — HID + SetupAPI for enumeration.
target_link_libraries(TitanInputPlugin PRIVATE
    setupapi
    hid
)
