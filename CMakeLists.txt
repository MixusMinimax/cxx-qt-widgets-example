cmake_minimum_required(VERSION 3.14)

project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

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

cxx_qt_import_crate(
        MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/backend/Cargo.toml
        CRATES backend
        QT_MODULES Qt6::Core
)

add_executable(myapp
        ui/main.cpp
        ui/MainWindow.cpp
        ui/MainWindow.cpp
        ui/MainWindow.h
)

target_link_libraries(myapp
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        backend
)