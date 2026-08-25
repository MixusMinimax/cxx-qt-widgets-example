cmake_minimum_required(VERSION 3.14)

project(MyApp LANGUAGES CXX)

option(AUTOLINK_COMPILE_COMMANDS "\
symlink the compile_commands.json file from the build folder into the root folder. \
clangd only checks in the root folder and the /build folder, \
but in CLion, the build folder is called /cmake-build-debug." OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
)

# CXX-Qt integration
include(FetchContent)

find_package(CxxQt QUIET)

if (NOT CxxQt_FOUND)
    FetchContent_Declare(
            CxxQt
            GIT_REPOSITORY https://github.com/kdab/cxx-qt-cmake.git
            GIT_TAG 0.9.1
    )

    FetchContent_MakeAvailable(CxxQt)
endif ()

qt_standard_project_setup()

add_subdirectory(vendor)

cxx_qt_import_crate(
        MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/backend/Cargo.toml
        CRATES backend
        QT_MODULES Qt6::Core
)

qt_add_executable(myapp
        ui/main.cpp
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/MainWindow.ui
        ui/GraphWidget.cpp
        ui/GraphWidget.h
)

target_link_libraries(myapp
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        backend
        qcustomplot
)

if (AUTOLINK_COMPILE_COMMANDS)
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif ()