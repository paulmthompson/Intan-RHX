
add_executable(spikescopechannelname_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/spikescopechannelname_golden_test.cpp
    ${_engine}/spikescopechannelname.cpp)
set_target_properties(spikescopechannelname_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(spikescopechannelname_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(spikescopechannelname_golden_test PRIVATE
    ${_engine}
    ${_api_hardware}
    ${_includes})
target_compile_options(spikescopechannelname_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(spikescopechannelname_golden_test PRIVATE -Wl,--gc-sections)
