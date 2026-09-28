cmake_minimum_required(VERSION 3.24)
project(Intan-RHX VERSION 3.2.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#[[
Best practice for looking for cmake modules cmake folder in root of the project
Currently, this project does not have any cmake modules
From Professional CMake Chapter 11 Modules
]]
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS MultimediaWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS UiTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR}Xml)

find_package(OpenCL REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS OpenGL)

#Create Library
set(Sources
        main.cpp
        Engine/API/Synthetic/playbackrhxcontroller.cpp
        Engine/API/Synthetic/randomnumber.cpp
        Engine/API/Synthetic/synthdatablockgenerator.cpp
        Engine/API/Synthetic/syntheticrhxcontroller.cpp
        Engine/API/Abstract/abstractrhxcontroller.cpp 
        Engine/API/Hardware/rhxcontroller.cpp 
        Engine/API/Hardware/rhxdatablock.cpp 
        Engine/API/Hardware/rhxregisters.cpp 
        Engine/Processing/DataFileReaders/datafile.cpp 
        Engine/Processing/DataFileReaders/datafilemanager.cpp 
        Engine/Processing/DataFileReaders/datafilereader.cpp 
        Engine/Processing/DataFileReaders/fileperchannelmanager.cpp 
        Engine/Processing/DataFileReaders/filepersignaltypemanager.cpp 
        Engine/Processing/DataFileReaders/traditionalintanfilemanager.cpp 
        Engine/Processing/SaveManagers/fileperchannelsavemanager.cpp 
        Engine/Processing/SaveManagers/filepersignaltypesavemanager.cpp 
        Engine/Processing/SaveManagers/intanfilesavemanager.cpp 
        Engine/Processing/SaveManagers/savefile.cpp 
        Engine/Processing/SaveManagers/savemanager.cpp 
        Engine/Processing/XPUInterfaces/abstractxpuinterface.cpp 
        Engine/Processing/XPUInterfaces/cpuinterface.cpp 
        Engine/Processing/XPUInterfaces/gpuinterface.cpp 
        Engine/Processing/XPUInterfaces/xpucontroller.cpp 
        Engine/Processing/channel.cpp 
        Engine/Processing/commandparser.cpp 
        Engine/Processing/controllerinterface.cpp 
        Engine/Processing/datastreamfifo.cpp 
        Engine/Processing/displayundomanager.cpp 
        Engine/Processing/fastfouriertransform.cpp 
        Engine/Processing/filter.cpp 
        Engine/Processing/matfilewriter.cpp 
        Engine/Processing/rhxdatareader.cpp 
        Engine/Processing/signalsources.cpp 
        Engine/Processing/softwarereferenceprocessor.cpp 
        Engine/Processing/stateitem.cpp 
        Engine/Processing/stimparameters.cpp 
        Engine/Processing/stimparametersclipboard.cpp 
        Engine/Processing/systemstate.cpp 
        Engine/Processing/tcpcommunicator.cpp 
        Engine/Processing/waveformfifo.cpp 
        Engine/Processing/impedancereader.cpp 
        Engine/Processing/xmlinterface.cpp 
        Engine/Threads/audiothread.cpp 
        Engine/Threads/savetodiskthread.cpp 
        Engine/Threads/tcpdataoutputthread.cpp 
        Engine/Threads/usbdatathread.cpp 
        Engine/Threads/waveformprocessorthread.cpp 
        GUI/Dialogs/advancedstartupdialog.cpp 
        GUI/Dialogs/ampsettledialog.cpp 
        GUI/Dialogs/analogoutconfigdialog.cpp 
        GUI/Dialogs/anoutdialog.cpp 
        GUI/Dialogs/autocolordialog.cpp 
        GUI/Dialogs/autogroupdialog.cpp 
        GUI/Dialogs/auxdigoutconfigdialog.cpp 
        GUI/Dialogs/bandwidthdialog.cpp 
        GUI/Dialogs/boardselectdialog.cpp 
        GUI/Dialogs/cabledelaydialog.cpp 
        GUI/Dialogs/chargerecoverydialog.cpp 
        GUI/Dialogs/demodialog.cpp 
        GUI/Dialogs/digoutdialog.cpp 
        GUI/Dialogs/isidialog.cpp 
        GUI/Dialogs/keyboardshortcutdialog.cpp 
        GUI/Dialogs/performanceoptimizationdialog.cpp 
        GUI/Dialogs/playbackfilepositiondialog.cpp 
        GUI/Dialogs/psthdialog.cpp 
        GUI/Dialogs/referenceselectdialog.cpp 
        GUI/Dialogs/renamechanneldialog.cpp 
        GUI/Dialogs/scrollablemessageboxdialog.cpp 
        GUI/Dialogs/setfileformatdialog.cpp 
        GUI/Dialogs/setthresholdsdialog.cpp 
        GUI/Dialogs/spectrogramdialog.cpp 
        GUI/Dialogs/spikesortingdialog.cpp 
        GUI/Dialogs/startupdialog.cpp 
        GUI/Dialogs/stimparamdialog.cpp 
        GUI/Dialogs/triggerrecorddialog.cpp 
        GUI/Dialogs/waveformselectdialog.cpp 
        GUI/Widgets/abstractfigure.cpp 
        GUI/Widgets/anoutfigure.cpp 
        GUI/Widgets/controlpanelaudioanalogtab.cpp 
        GUI/Widgets/controlpanelbandwidthtab.cpp 
        GUI/Widgets/controlpanelconfiguretab.cpp 
        GUI/Widgets/controlpanelimpedancetab.cpp 
        GUI/Widgets/controlpaneltriggertab.cpp 
        GUI/Widgets/digfigure.cpp 
        GUI/Widgets/displayedwaveform.cpp 
        GUI/Widgets/displaylistmanager.cpp 
        GUI/Widgets/filterdisplayselector.cpp 
        GUI/Widgets/filterplot.cpp 
        GUI/Widgets/impedancegradient.cpp 
        GUI/Widgets/isiplot.cpp 
        GUI/Widgets/multicolumndisplay.cpp 
        GUI/Widgets/multiwaveformplot.cpp 
        GUI/Widgets/pageview.cpp 
        GUI/Widgets/plotutilities.cpp 
        GUI/Widgets/psthplot.cpp 
        GUI/Widgets/scrollbar.cpp 
        GUI/Widgets/smartspinbox.cpp 
        GUI/Widgets/spikegradient.cpp 
        GUI/Widgets/spikeplot.cpp 
        GUI/Widgets/statusbars.cpp 
        GUI/Widgets/stimfigure.cpp 
        GUI/Widgets/tcpdisplay.cpp 
        GUI/Widgets/voltagespinbox.cpp 
        GUI/Widgets/waveformdisplaycolumn.cpp 
        GUI/Widgets/waveformdisplaymanager.cpp 
        GUI/Windows/controlwindow.cpp 
        GUI/Windows/probemapwindow.cpp 
        GUI/Dialogs/impedancefreqdialog.cpp 
        GUI/Widgets/controlpanel.cpp 
        GUI/Widgets/spectrogramplot.cpp 
        GUI/Windows/viewfilterswindow.cpp
)

set(Headers 
        Engine/API/Synthetic/playbackrhxcontroller.h 
        Engine/API/Synthetic/randomnumber.h 
        Engine/API/Synthetic/synthdatablockgenerator.h 
        Engine/API/Synthetic/syntheticrhxcontroller.h 
        Engine/API/Abstract/abstractrhxcontroller.h 
        Engine/API/Hardware/rhxcontroller.h 
        Engine/API/Hardware/rhxdatablock.h 
        Engine/API/Hardware/rhxglobals.h 
        Engine/API/Hardware/rhxregisters.h 
        Engine/Processing/DataFileReaders/datafile.h 
        Engine/Processing/DataFileReaders/datafilemanager.h 
        Engine/Processing/DataFileReaders/datafilereader.h 
        Engine/Processing/DataFileReaders/fileperchannelmanager.h 
        Engine/Processing/DataFileReaders/filepersignaltypemanager.h 
        Engine/Processing/DataFileReaders/traditionalintanfilemanager.h 
        Engine/Processing/SaveManagers/fileperchannelsavemanager.h 
        Engine/Processing/SaveManagers/filepersignaltypesavemanager.h 
        Engine/Processing/SaveManagers/intanfilesavemanager.h 
        Engine/Processing/SaveManagers/savefile.h 
        Engine/Processing/SaveManagers/savemanager.h 
        Engine/Processing/XPUInterfaces/abstractxpuinterface.h 
        Engine/Processing/XPUInterfaces/cpuinterface.h 
        Engine/Processing/XPUInterfaces/gpuinterface.h 
        Engine/Processing/XPUInterfaces/xpucontroller.h 
        Engine/Processing/channel.h 
        Engine/Processing/commandparser.h 
        Engine/Processing/controllerinterface.h 
        Engine/Processing/datastreamfifo.h 
        Engine/Processing/displayundomanager.h 
        Engine/Processing/fastfouriertransform.h 
        Engine/Processing/filter.h 
        Engine/Processing/matfilewriter.h 
        Engine/Processing/minmax.h 
        Engine/Processing/probemapdatastructures.h 
        Engine/Processing/rhxdatareader.h 
        Engine/Processing/semaphore.h 
        Engine/Processing/signalsources.h 
        Engine/Processing/softwarereferenceprocessor.h 
        Engine/Processing/stateitem.h 
        Engine/Processing/stimparameters.h 
        Engine/Processing/stimparametersclipboard.h 
        Engine/Processing/systemstate.h 
        Engine/Processing/tcpcommunicator.h 
        Engine/Processing/waveformfifo.h 
        Engine/Processing/impedancereader.h 
        Engine/Processing/xmlinterface.h 
        Engine/Threads/audiothread.h 
        Engine/Threads/savetodiskthread.h 
        Engine/Threads/tcpdataoutputthread.h 
        Engine/Threads/usbdatathread.h 
        Engine/Threads/waveformprocessorthread.h 
        GUI/Dialogs/advancedstartupdialog.h 
        GUI/Dialogs/ampsettledialog.h 
        GUI/Dialogs/analogoutconfigdialog.h 
        GUI/Dialogs/anoutdialog.h 
        GUI/Dialogs/autocolordialog.h 
        GUI/Dialogs/autogroupdialog.h 
        GUI/Dialogs/auxdigoutconfigdialog.h 
        GUI/Dialogs/bandwidthdialog.h 
        GUI/Dialogs/boardselectdialog.h 
        GUI/Dialogs/cabledelaydialog.h 
        GUI/Dialogs/chargerecoverydialog.h 
        GUI/Dialogs/demodialog.h 
        GUI/Dialogs/digoutdialog.h 
        GUI/Dialogs/isidialog.h 
        GUI/Dialogs/keyboardshortcutdialog.h 
        GUI/Dialogs/performanceoptimizationdialog.h 
        GUI/Dialogs/playbackfilepositiondialog.h 
        GUI/Dialogs/psthdialog.h 
        GUI/Dialogs/referenceselectdialog.h 
        GUI/Dialogs/renamechanneldialog.h 
        GUI/Dialogs/scrollablemessageboxdialog.h 
        GUI/Dialogs/setfileformatdialog.h 
        GUI/Dialogs/setthresholdsdialog.h 
        GUI/Dialogs/spectrogramdialog.h 
        GUI/Dialogs/spikesortingdialog.h 
        GUI/Dialogs/startupdialog.h 
        GUI/Dialogs/stimparamdialog.h 
        GUI/Dialogs/triggerrecorddialog.h 
        GUI/Dialogs/waveformselectdialog.h 
        GUI/Widgets/abstractfigure.h 
        GUI/Widgets/anoutfigure.h 
        GUI/Widgets/controlpanelaudioanalogtab.h 
        GUI/Widgets/controlpanelbandwidthtab.h 
        GUI/Widgets/controlpanelconfiguretab.h 
        GUI/Widgets/controlpanelimpedancetab.h 
        GUI/Widgets/controlpaneltriggertab.h 
        GUI/Widgets/digfigure.h 
        GUI/Widgets/displayedwaveform.h 
        GUI/Widgets/displaylistmanager.h 
        GUI/Widgets/filterdisplayselector.h 
        GUI/Widgets/filterplot.h 
        GUI/Widgets/impedancegradient.h 
        GUI/Widgets/isiplot.h 
        GUI/Widgets/multicolumndisplay.h 
        GUI/Widgets/multiwaveformplot.h 
        GUI/Widgets/pageview.h 
        GUI/Widgets/plotutilities.h 
        GUI/Widgets/psthplot.h 
        GUI/Widgets/scrollbar.h 
        GUI/Widgets/smartspinbox.h 
        GUI/Widgets/spikegradient.h 
        GUI/Widgets/spikeplot.h 
        GUI/Widgets/statusbars.h 
        GUI/Widgets/stimfigure.h 
        GUI/Widgets/tcpdisplay.h 
        GUI/Widgets/voltagespinbox.h 
        GUI/Widgets/waveformdisplaycolumn.h 
        GUI/Widgets/waveformdisplaymanager.h 
        GUI/Windows/controlwindow.h 
        GUI/Windows/probemapwindow.h 
        GUI/Dialogs/impedancefreqdialog.h 
        GUI/Widgets/controlpanel.h 
        GUI/Widgets/spectrogramplot.h 
        GUI/Windows/viewfilterswindow.h
)


#[[
The CTest module helps with several testing functions.
It defines the BUILD_TESTING cache variable that defaults to true, and also automatically calls enable_testing(), so this does not need to be called manually
CTest should be included early in the top level CMakeLists.txt file
Professional CMake 25.10.3
]]
include(CTest)

if (WIN32)
        if (MSVC)
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
                message(STATUS "This is a Debug build for MSVC, enabling address sanitizer")
        endif()
endif()

if (UNIX)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                message(STATUS "This is a Debug build for Clang, enabling address sanitizer")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GCC")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
        endif()
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(IntanRHX
        MANUAL_FINALIZATION
        ${Sources}
        ${Headers}
        IntanRHX.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET IntanRHX APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    add_executable(IntanRHX
        ${Sources}
        ${Headers}
        IntanRHX.qrc
    )
endif()

target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::MultimediaWidgets)
target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::OpenGL)
target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::UiTools)
target_link_libraries(IntanRHX PRIVATE Qt${QT_VERSION_MAJOR}::Xml)

target_link_libraries(IntanRHX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libraries/Linux/libokFrontPanel.so")
target_link_libraries(IntanRHX PRIVATE OpenCL::OpenCL)

target_include_directories(IntanRHX PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/includes>"

    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Processing>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Processing/DataFileReaders>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Processing/SaveManagers>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Processing/XPUInterfaces>"

    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/Threads>"

    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/API/Synthetic>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/API/Abstract>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Engine/API/Hardware>"

    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GUI/Dialogs>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GUI/Widgets>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GUI/Windows>"

    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_custom_target(fpga_bitfiles
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/FPGA-bitfiles ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(open_cl_kernel
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/kernel.cl ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(IntanRHX fpga_bitfiles open_cl_kernel)
