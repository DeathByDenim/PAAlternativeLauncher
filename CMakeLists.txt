project(PAAlternativeLauncher)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(PAAlternativeLauncher_SRCS PAAlternativeLauncher.cpp main.cpp)
qt4_automoc(${PAAlternativeLauncher_SRCS})
add_executable(PAAlternativeLauncher ${PAAlternativeLauncher_SRCS})
target_link_libraries(PAAlternativeLauncher ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
install(TARGETS PAAlternativeLauncher RUNTIME DESTINATION bin)
