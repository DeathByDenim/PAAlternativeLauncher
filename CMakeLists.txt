project(PAAlternativeLauncher)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
find_package(ZLIB REQUIRED)
find_package(QJSON REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR} ${QJSON_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(PAAlternativeLauncher_SRCS information.cpp advanceddialog.cpp bundle.cpp sha1.cpp patcher.cpp PAAlternativeLauncher.cpp main.cpp)
qt4_automoc(${PAAlternativeLauncher_SRCS})

QT4_ADD_RESOURCES(PAAlternativeLauncher_RCC_SRCS PAAlternativeLauncher.qrc)

add_executable(PAAlternativeLauncher ${PAAlternativeLauncher_SRCS} ${PAAlternativeLauncher_RCC_SRCS})
target_link_libraries(PAAlternativeLauncher ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARIES} ${ZLIB_LIBRARY} ${QJSON_LIBRARIES})
install(TARGETS PAAlternativeLauncher RUNTIME DESTINATION bin)
