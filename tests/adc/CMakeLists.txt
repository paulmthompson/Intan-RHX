
add_executable(boardadcspikedetection_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/boardadcspikedetection_golden_test.cpp
    ${_engine}/boardadcspikedetection.cpp
    ${_engine}/boardadcscaling.cpp)
set_target_properties(boardadcspikedetection_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(boardadcspikedetection_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(boardadcspikedetection_golden_test PRIVATE
    ${_engine}
    ${_includes})
target_compile_options(boardadcspikedetection_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(boardadcspikedetection_golden_test PRIVATE -Wl,--gc-sections)

add_executable(boardadcscaling_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/boardadcscaling_golden_test.cpp
    ${_engine}/boardadcscaling.cpp)
set_target_properties(boardadcscaling_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(boardadcscaling_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(boardadcscaling_golden_test PRIVATE
    ${_engine}
    ${_includes})
target_compile_options(boardadcscaling_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(boardadcscaling_golden_test PRIVATE -Wl,--gc-sections)