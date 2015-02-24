project(paalternativelauncher)
cmake_minimum_required(VERSION 2.8.11)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)


#find_package(Qt5 REQUIRED Widgets Network)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR})

set(paalternativelauncher_SRCS bundle.cpp patcher.cpp paalternativelauncher.cpp main.cpp information.cpp advanceddialog.cpp)
qt5_add_resources(paalternativelauncher_RCC_SRCS paalternativelauncher.qrc)

add_executable(paalternativelauncher ${paalternativelauncher_SRCS} ${paalternativelauncher_RCC_SRCS})

target_link_libraries(paalternativelauncher Qt5::Widgets Qt5::Network Qt5::Concurrent ${ZLIB_LIBRARY})
