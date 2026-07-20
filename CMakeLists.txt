cmake_minimum_required(VERSION 3.25)

project(pixelator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Qml Quick QuickControls2 Concurrent)
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(pixelator
    main.cpp
    pixelworker.cpp
    pixelworker.h
)

qt_add_qml_module(pixelator
    URI Pixelator
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(pixelator PRIVATE
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Concurrent
    ${OpenCV_LIBS}
)

target_include_directories(pixelator PRIVATE ${OpenCV_INCLUDE_DIRS})

set_target_properties(pixelator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS pixelator
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES packaging/pixelator.desktop DESTINATION share/applications)
install(FILES packaging/pixelator.svg DESTINATION share/icons/hicolor/scalable/apps)
