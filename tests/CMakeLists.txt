# Host golden tests (Linux only; see root CMakeLists.txt).

set(_engine "${CMAKE_SOURCE_DIR}/Engine/Processing")
set(_includes "${CMAKE_SOURCE_DIR}/includes")
set(_api_hardware "${CMAKE_SOURCE_DIR}/Engine/API/Hardware")
set(_api_abstract "${CMAKE_SOURCE_DIR}/Engine/API/Abstract")

add_executable(rhsdigoutconfiguration_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/rhsdigoutconfiguration_golden_test.cpp
    ${_engine}/rhsdigoutconfiguration.cpp)
set_target_properties(rhsdigoutconfiguration_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(rhsdigoutconfiguration_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(rhsdigoutconfiguration_golden_test PRIVATE
    ${_engine}
    ${_api_hardware}
    ${_api_abstract}
    ${_includes})
target_compile_options(rhsdigoutconfiguration_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(rhsdigoutconfiguration_golden_test PRIVATE -Wl,--gc-sections)

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

add_executable(antidromicspikedetection_golden_test
    ${CMAKE_CURRENT_SOURCE_DIR}/antidromicspikedetection_golden_test.cpp
    ${_engine}/antidromicspikedetection.cpp)
set_target_properties(antidromicspikedetection_golden_test PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(antidromicspikedetection_golden_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(antidromicspikedetection_golden_test PRIVATE
    ${_engine}
    ${_includes})
target_compile_options(antidromicspikedetection_golden_test PRIVATE
    -ffunction-sections -fdata-sections)
target_link_options(antidromicspikedetection_golden_test PRIVATE -Wl,--gc-sections)
