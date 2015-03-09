project(paalternativelauncher)
cmake_minimum_required(VERSION 2.8.11)

set(MAJOR_VERSION "0")
set(MINOR_VERSION "5")
set(PATCH_VERSION "1")

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

if(WIN32)
	set(CMAKE_CXX_FLAGS "-mwindows")
	set(paalternativelauncher_RCC_SRCS "paalternativelauncher.rc")
endif(WIN32)

include_directories(${Qt5Core_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR})

set(paalternativelauncher_SRCS moddatabaseframe.cpp bundle.cpp patcher.cpp paalternativelauncher.cpp main.cpp information.cpp advanceddialog.cpp)
qt5_add_resources(paalternativelauncher_RCC_SRCS paalternativelauncher.qrc)

add_executable(paalternativelauncher ${paalternativelauncher_SRCS} ${paalternativelauncher_RCC_SRCS})

target_link_libraries(paalternativelauncher Qt5::Widgets Qt5::Network Qt5::Concurrent ${ZLIB_LIBRARY})

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
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt5-default, zlib1g, libc6")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/games")
	set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_STRIP_FILES games/paalternativelauncher)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/fixup_deb.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/fixup_deb.sh" @ONLY IMMEDIATE)

	include(CPack)
endif(UNIX)
