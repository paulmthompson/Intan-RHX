
Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.14.0
)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(Catch)


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

add_subdirectory(adc)

add_subdirectory(pulseresistance)

add_subdirectory(antidromic)

add_subdirectory(spikescope)