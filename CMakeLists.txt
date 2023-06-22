cmake_minimum_required(VERSION 3.16)
project(IntanRHX VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up AUTOMOC and some sensible defaults for runtime execution
# When using Qt 6.3, you can replace the code block below with
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Multimedia Network Widgets Xml)

qt_add_executable(IntanRHX WIN32 MACOSX_BUNDLE
    Engine/API/Abstract/abstractrhxcontroller.cpp Engine/API/Abstract/abstractrhxcontroller.h
    Engine/API/Hardware/rhxcontroller.cpp Engine/API/Hardware/rhxcontroller.h
    Engine/API/Hardware/rhxdatablock.cpp Engine/API/Hardware/rhxdatablock.h
    Engine/API/Hardware/rhxglobals.h
    Engine/API/Hardware/rhxregisters.cpp Engine/API/Hardware/rhxregisters.h
    Engine/API/Synthetic/playbackrhxcontroller.cpp Engine/API/Synthetic/playbackrhxcontroller.h
    Engine/API/Synthetic/randomnumber.cpp Engine/API/Synthetic/randomnumber.h
    Engine/API/Synthetic/synthdatablockgenerator.cpp Engine/API/Synthetic/synthdatablockgenerator.h
    Engine/API/Synthetic/syntheticrhxcontroller.cpp Engine/API/Synthetic/syntheticrhxcontroller.h
    Engine/Processing/DataFileReaders/datafile.cpp Engine/Processing/DataFileReaders/datafile.h
    Engine/Processing/DataFileReaders/datafilemanager.cpp Engine/Processing/DataFileReaders/datafilemanager.h
    Engine/Processing/DataFileReaders/datafilereader.cpp Engine/Processing/DataFileReaders/datafilereader.h
    Engine/Processing/DataFileReaders/fileperchannelmanager.cpp Engine/Processing/DataFileReaders/fileperchannelmanager.h
    Engine/Processing/DataFileReaders/filepersignaltypemanager.cpp Engine/Processing/DataFileReaders/filepersignaltypemanager.h
    Engine/Processing/DataFileReaders/traditionalintanfilemanager.cpp Engine/Processing/DataFileReaders/traditionalintanfilemanager.h
    Engine/Processing/SaveManagers/fileperchannelsavemanager.cpp Engine/Processing/SaveManagers/fileperchannelsavemanager.h
    Engine/Processing/SaveManagers/filepersignaltypesavemanager.cpp Engine/Processing/SaveManagers/filepersignaltypesavemanager.h
    Engine/Processing/SaveManagers/intanfilesavemanager.cpp Engine/Processing/SaveManagers/intanfilesavemanager.h
    Engine/Processing/SaveManagers/savefile.cpp Engine/Processing/SaveManagers/savefile.h
    Engine/Processing/SaveManagers/savemanager.cpp Engine/Processing/SaveManagers/savemanager.h
    Engine/Processing/XPUInterfaces/abstractxpuinterface.cpp Engine/Processing/XPUInterfaces/abstractxpuinterface.h
    Engine/Processing/XPUInterfaces/cpuinterface.cpp Engine/Processing/XPUInterfaces/cpuinterface.h
    Engine/Processing/XPUInterfaces/gpuinterface.cpp Engine/Processing/XPUInterfaces/gpuinterface.h
    Engine/Processing/XPUInterfaces/xpucontroller.cpp Engine/Processing/XPUInterfaces/xpucontroller.h
    Engine/Processing/channel.cpp Engine/Processing/channel.h
    Engine/Processing/commandparser.cpp Engine/Processing/commandparser.h
    Engine/Processing/controllerinterface.cpp Engine/Processing/controllerinterface.h
    Engine/Processing/datastreamfifo.cpp Engine/Processing/datastreamfifo.h
    Engine/Processing/displayundomanager.cpp Engine/Processing/displayundomanager.h
    Engine/Processing/fastfouriertransform.cpp Engine/Processing/fastfouriertransform.h
    Engine/Processing/filter.cpp Engine/Processing/filter.h
    Engine/Processing/impedancereader.cpp Engine/Processing/impedancereader.h
    Engine/Processing/matfilewriter.cpp Engine/Processing/matfilewriter.h
    Engine/Processing/minmax.h
    Engine/Processing/probemapdatastructures.h
    Engine/Processing/rhxdatareader.cpp Engine/Processing/rhxdatareader.h
    Engine/Processing/semaphore.h
    Engine/Processing/signalsources.cpp Engine/Processing/signalsources.h
    Engine/Processing/softwarereferenceprocessor.cpp Engine/Processing/softwarereferenceprocessor.h
    Engine/Processing/stateitem.cpp Engine/Processing/stateitem.h
    Engine/Processing/stimparameters.cpp Engine/Processing/stimparameters.h
    Engine/Processing/stimparametersclipboard.cpp Engine/Processing/stimparametersclipboard.h
    Engine/Processing/systemstate.cpp Engine/Processing/systemstate.h
    Engine/Processing/tcpcommunicator.cpp Engine/Processing/tcpcommunicator.h
    Engine/Processing/waveformfifo.cpp Engine/Processing/waveformfifo.h
    Engine/Processing/xmlinterface.cpp Engine/Processing/xmlinterface.h
    Engine/Threads/audiothread.cpp Engine/Threads/audiothread.h
    Engine/Threads/savetodiskthread.cpp Engine/Threads/savetodiskthread.h
    Engine/Threads/tcpdataoutputthread.cpp Engine/Threads/tcpdataoutputthread.h
    Engine/Threads/usbdatathread.cpp Engine/Threads/usbdatathread.h
    Engine/Threads/waveformprocessorthread.cpp Engine/Threads/waveformprocessorthread.h
    GUI/Dialogs/advancedstartupdialog.cpp GUI/Dialogs/advancedstartupdialog.h
    GUI/Dialogs/ampsettledialog.cpp GUI/Dialogs/ampsettledialog.h
    GUI/Dialogs/analogoutconfigdialog.cpp GUI/Dialogs/analogoutconfigdialog.h
    GUI/Dialogs/anoutdialog.cpp GUI/Dialogs/anoutdialog.h
    GUI/Dialogs/autocolordialog.cpp GUI/Dialogs/autocolordialog.h
    GUI/Dialogs/autogroupdialog.cpp GUI/Dialogs/autogroupdialog.h
    GUI/Dialogs/auxdigoutconfigdialog.cpp GUI/Dialogs/auxdigoutconfigdialog.h
    GUI/Dialogs/bandwidthdialog.cpp GUI/Dialogs/bandwidthdialog.h
    GUI/Dialogs/boardselectdialog.cpp GUI/Dialogs/boardselectdialog.h
    GUI/Dialogs/cabledelaydialog.cpp GUI/Dialogs/cabledelaydialog.h
    GUI/Dialogs/chargerecoverydialog.cpp GUI/Dialogs/chargerecoverydialog.h
    GUI/Dialogs/demodialog.cpp GUI/Dialogs/demodialog.h
    GUI/Dialogs/digoutdialog.cpp GUI/Dialogs/digoutdialog.h
    GUI/Dialogs/impedancefreqdialog.cpp GUI/Dialogs/impedancefreqdialog.h
    GUI/Dialogs/isidialog.cpp GUI/Dialogs/isidialog.h
    GUI/Dialogs/keyboardshortcutdialog.cpp GUI/Dialogs/keyboardshortcutdialog.h
    GUI/Dialogs/performanceoptimizationdialog.cpp GUI/Dialogs/performanceoptimizationdialog.h
    GUI/Dialogs/playbackfilepositiondialog.cpp GUI/Dialogs/playbackfilepositiondialog.h
    GUI/Dialogs/psthdialog.cpp GUI/Dialogs/psthdialog.h
    GUI/Dialogs/referenceselectdialog.cpp GUI/Dialogs/referenceselectdialog.h
    GUI/Dialogs/renamechanneldialog.cpp GUI/Dialogs/renamechanneldialog.h
    GUI/Dialogs/scrollablemessageboxdialog.cpp GUI/Dialogs/scrollablemessageboxdialog.h
    GUI/Dialogs/setfileformatdialog.cpp GUI/Dialogs/setfileformatdialog.h
    GUI/Dialogs/setthresholdsdialog.cpp GUI/Dialogs/setthresholdsdialog.h
    GUI/Dialogs/spectrogramdialog.cpp GUI/Dialogs/spectrogramdialog.h
    GUI/Dialogs/spikesortingdialog.cpp GUI/Dialogs/spikesortingdialog.h
    GUI/Dialogs/startupdialog.cpp GUI/Dialogs/startupdialog.h
    GUI/Dialogs/stimparamdialog.cpp GUI/Dialogs/stimparamdialog.h
    GUI/Dialogs/triggerrecorddialog.cpp GUI/Dialogs/triggerrecorddialog.h
    GUI/Dialogs/waveformselectdialog.cpp GUI/Dialogs/waveformselectdialog.h
    GUI/Widgets/abstractfigure.cpp GUI/Widgets/abstractfigure.h
    GUI/Widgets/anoutfigure.cpp GUI/Widgets/anoutfigure.h
    GUI/Widgets/controlpanel.cpp GUI/Widgets/controlpanel.h
    GUI/Widgets/controlpanelaudioanalogtab.cpp GUI/Widgets/controlpanelaudioanalogtab.h
    GUI/Widgets/controlpanelbandwidthtab.cpp GUI/Widgets/controlpanelbandwidthtab.h
    GUI/Widgets/controlpanelconfiguretab.cpp GUI/Widgets/controlpanelconfiguretab.h
    GUI/Widgets/controlpanelimpedancetab.cpp GUI/Widgets/controlpanelimpedancetab.h
    GUI/Widgets/controlpaneltriggertab.cpp GUI/Widgets/controlpaneltriggertab.h
    GUI/Widgets/digfigure.cpp GUI/Widgets/digfigure.h
    GUI/Widgets/displayedwaveform.cpp GUI/Widgets/displayedwaveform.h
    GUI/Widgets/displaylistmanager.cpp GUI/Widgets/displaylistmanager.h
    GUI/Widgets/filterdisplayselector.cpp GUI/Widgets/filterdisplayselector.h
    GUI/Widgets/filterplot.cpp GUI/Widgets/filterplot.h
    GUI/Widgets/impedancegradient.cpp GUI/Widgets/impedancegradient.h
    GUI/Widgets/isiplot.cpp GUI/Widgets/isiplot.h
    GUI/Widgets/multicolumndisplay.cpp GUI/Widgets/multicolumndisplay.h
    GUI/Widgets/multiwaveformplot.cpp GUI/Widgets/multiwaveformplot.h
    GUI/Widgets/pageview.cpp GUI/Widgets/pageview.h
    GUI/Widgets/plotutilities.cpp GUI/Widgets/plotutilities.h
    GUI/Widgets/psthplot.cpp GUI/Widgets/psthplot.h
    GUI/Widgets/scrollbar.cpp GUI/Widgets/scrollbar.h
    GUI/Widgets/smartspinbox.cpp GUI/Widgets/smartspinbox.h
    GUI/Widgets/spectrogramplot.cpp GUI/Widgets/spectrogramplot.h
    GUI/Widgets/spikegradient.cpp GUI/Widgets/spikegradient.h
    GUI/Widgets/spikeplot.cpp GUI/Widgets/spikeplot.h
    GUI/Widgets/statusbars.cpp GUI/Widgets/statusbars.h
    GUI/Widgets/stimfigure.cpp GUI/Widgets/stimfigure.h
    GUI/Widgets/tcpdisplay.cpp GUI/Widgets/tcpdisplay.h
    GUI/Widgets/voltagespinbox.cpp GUI/Widgets/voltagespinbox.h
    GUI/Widgets/waveformdisplaycolumn.cpp GUI/Widgets/waveformdisplaycolumn.h
    GUI/Widgets/waveformdisplaymanager.cpp GUI/Widgets/waveformdisplaymanager.h
    GUI/Windows/controlwindow.cpp GUI/Windows/controlwindow.h
    GUI/Windows/probemapwindow.cpp GUI/Windows/probemapwindow.h
    GUI/Windows/viewfilterswindow.cpp GUI/Windows/viewfilterswindow.h
    main.cpp
)
target_include_directories(IntanRHX PRIVATE
    /
    Engine/API/Abstract
    Engine/API/Hardware
    Engine/API/Synthetic
    Engine/Processing
    Engine/Processing/DataFileReaders
    Engine/Processing/SaveManagers
    Engine/Processing/XPUInterfaces
    Engine/Threads
    GUI/Dialogs
    GUI/Widgets
    GUI/Windows
    includes
)

target_compile_definitions(IntanRHX PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(IntanRHX PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt::Widgets
    Qt::Xml
)


# Resources:
set(IntanRHX_resource_files
    "images/IntanLogo_32x32_white_frame.png"
    "images/Intan_Icon_32p_trans24.png"
    "images/RHX_splash.png"
    "images/bestfit.png"
    "images/checkerboard.png"
    "images/choosefileformaticon.png"
    "images/clamp2_controller_board.png"
    "images/clamp8_controller_board.png"
    "images/collapseicon.png"
    "images/default.png"
    "images/downarrow.png"
    "images/expandicon.png"
    "images/fficon.png"
    "images/hideicon.png"
    "images/jumpback10s.png"
    "images/jumpback1s.png"
    "images/jumptoicon.png"
    "images/leftarrow.png"
    "images/lessspacingicon.png"
    "images/minusicon.png"
    "images/morespacingicon.png"
    "images/omega.png"
    "images/plusicon.png"
    "images/recordicon.png"
    "images/resize.png"
    "images/rewindicon.png"
    "images/rhd1024_controller_board.png"
    "images/rhd512_controller_board.png"
    "images/rightarrow.png"
    "images/runicon.png"
    "images/save_badge.png"
    "images/save_badge_selected.png"
    "images/scroll.png"
    "images/scroll_down_active_badge.png"
    "images/scroll_down_badge.png"
    "images/scroll_up_active_badge.png"
    "images/scroll_up_badge.png"
    "images/select.png"
    "images/selectfilenameicon.png"
    "images/showicon.png"
    "images/spike.png"
    "images/status_header.png"
    "images/stim_controller_board.png"
    "images/stim_enabled_badge.png"
    "images/stim_enabled_badge_selected.png"
    "images/stopicon.png"
    "images/tcp_badge.png"
    "images/tcp_badge_selected.png"
    "images/thumbtackdownicon.png"
    "images/tostarticon.png"
    "images/triggeredrecordicon.png"
    "images/unpin_badge_angle.png"
    "images/uparrow.png"
    "images/usb_interface_board.png"
    "images/zoomin.png"
    "images/zoomout.png"
)

qt_add_resources(IntanRHX "IntanRHX"
    PREFIX
        "/"
    FILES
        ${IntanRHX_resource_files}
)

if(WIN32)
    target_link_libraries(IntanRHX PRIVATE
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/libraries/Windows/
        OpenCL
        delayimp
        okFrontPanel
    )
endif()

if(APPLE)
    target_link_libraries(IntanRHX PRIVATE
        "-framework OpenCL"
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/libraries/Mac/
        okFrontPanel
    )
endif()

if(UNIX AND NOT MACOS)
    target_link_libraries(IntanRHX PRIVATE
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/libraries/Linux/
        OpenCL
        okFrontPanel
    )
endif()

install(TARGETS IntanRHX
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Consider using qt_generate_deploy_app_script() for app deployment if
# the project can use Qt 6.3. In that case rerun qmake2cmake with
# --min-qt-version=6.3.
