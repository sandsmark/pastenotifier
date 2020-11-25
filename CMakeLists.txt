cmake_minimum_required(VERSION 3.5)

project(pastenotifier LANGUAGES CXX)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KF5GlobalAccel REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(pastenotifier main.cpp widget.cpp widget.h)

target_link_libraries(pastenotifier PRIVATE KF5::GlobalAccel Qt5::Widgets)
install(TARGETS pastenotifier DESTINATION bin)
install(FILES pastenotifier.desktop DESTINATION /etc/xdg/autostart/)
