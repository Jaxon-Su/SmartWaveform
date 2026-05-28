cmake_minimum_required(VERSION 3.16)

project(SmartWaveform VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

set(QT_NO_PRIVATE_MODULE_WARNING ON)
add_subdirectory(third_party/QXlsx)

qt_add_executable(SmartWaveform WIN32
    main.cpp
    src/models/ocrresult.h
    src/services/imagescanner.h
    src/services/imagescanner.cpp
    src/services/ollamaclient.h
    src/services/ollamaclient.cpp
    src/services/waveformocrparser.h
    src/services/waveformocrparser.cpp
    src/exporters/iresultwriter.h
    src/exporters/excelresultwriter.h
    src/exporters/excelresultwriter.cpp
    src/viewmodels/mainwindowviewmodel.h
    src/viewmodels/mainwindowviewmodel.cpp
    src/views/mainwindow.h
    src/views/mainwindow.cpp
)

target_include_directories(SmartWaveform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SmartWaveform
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        QXlsx::QXlsx
)

if(WIN32)
    get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

    add_custom_command(TARGET SmartWaveform POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                --compiler-runtime
                --no-opengl-sw
                --no-system-d3d-compiler
                --no-system-dxc-compiler
                --no-translations
                "$<TARGET_FILE:SmartWaveform>"
        COMMENT "Deploying Qt runtime with windeployqt"
    )
endif()
