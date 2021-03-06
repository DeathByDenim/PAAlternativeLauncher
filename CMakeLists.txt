project(paalternativelauncher)
cmake_minimum_required(VERSION 2.8.11)

set(MAJOR_VERSION "1")
set(MINOR_VERSION "0")
set(PATCH_VERSION "3")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
endif(APPLE)

find_package(Qt5 COMPONENTS Widgets Network Concurrent REQUIRED)

if(UNIX)
	find_package(Qt5 COMPONENTS DBus REQUIRED)
endif(UNIX)
if(WIN32)
	find_package(Qt5 COMPONENTS WinExtras REQUIRED)
endif(WIN32)
find_package(ZLIB REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(WIN32)
	set(CMAKE_CXX_FLAGS "-mwindows")
	set(paalternativelauncher_RCC_SRCS "paalternativelauncher.rc")
endif(WIN32)

#include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Concurrent_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR})
#if(UNIX)
#	 include_directories(${Qt5DBus_INCLUDE_DIRS})
#endif(UNIX)

set(paalternativelauncher_SRCS moddatabaseframe.cpp bundle.cpp patcher.cpp paalternativelauncher.cpp main.cpp information.cpp advanceddialog.cpp)
set(paalternativelauncher_RCC_SRCS paalternativelauncher.qrc)
qt5_add_resources(paalternativelauncher_RCC_SRCS paalternativelauncher.qrc)

add_executable(paalternativelauncher ${paalternativelauncher_SRCS} ${paalternativelauncher_RCC_SRCS})

target_link_libraries(paalternativelauncher Qt5::Widgets Qt5::Network Qt5::Concurrent ${ZLIB_LIBRARY})
if(UNIX)
	target_link_libraries(paalternativelauncher Qt5::DBus)
endif(UNIX)
if(WIN32)
	target_link_libraries(paalternativelauncher Qt5::WinExtras)
endif(WIN32)

install(TARGETS paalternativelauncher RUNTIME DESTINATION games)
install(FILES paalternativelauncher.desktop DESTINATION share/applications)
install(FILES img/icon.png DESTINATION share/paalternativelauncher/icon)

if(UNIX)
	install(FILES deb/copyright deb/changelog.gz deb/README DESTINATION share/doc/paalternativelauncher)

	set(CPACK_GENERATOR "DEB")
	set(CPACK_PACKAGE_FILE_NAME "paalternativeinstaller")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Alternative launcher for Planetary Annihilation.")
	set(CPACK_PACKAGE_VENDOR "DeathByDenim")
	set(CPACK_PACKAGE_CONTACT "DeathByDenim <jarno@jarno.ca>")
	set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
	set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.2), libqt5gui5 (>= 5.2), libqt5network5 (>= 5.2), libqt5widgets5 (>= 5.2), libqt5dbus5 (>= 5.2), zlib1g, libc6")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/games")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_STRIP_FILES games/paalternativelauncher)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/fixup_deb.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/fixup_deb.sh" @ONLY IMMEDIATE)

	include(CPack)
endif(UNIX)
