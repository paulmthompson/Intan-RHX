


add_executable(antidromicspikedetection_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/antidromicspikedetection_golden_test.cpp
    ${_engine}/Antidromic/antidromicspikedetection.cpp)
set_target_properties(antidromicspikedetection_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(antidromicspikedetection_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(antidromicspikedetection_golden_test PRIVATE
    ${_engine}
    ${_includes})
target_compile_options(antidromicspikedetection_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(antidromicspikedetection_golden_test PRIVATE -Wl,--gc-sections)
