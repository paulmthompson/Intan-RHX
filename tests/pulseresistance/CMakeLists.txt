

add_executable(pulseelectroderesistance_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/pulseelectroderesistance_golden_test.cpp
    ${_engine}/pulseelectroderesistance.cpp)
set_target_properties(pulseelectroderesistance_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(pulseelectroderesistance_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(pulseelectroderesistance_golden_test PRIVATE
    ${_engine}
    ${_api_hardware}
    ${_api_abstract}
    ${_includes})
target_compile_options(pulseelectroderesistance_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(pulseelectroderesistance_golden_test PRIVATE -Wl,--gc-sections)