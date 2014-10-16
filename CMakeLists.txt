project(paalternativelauncher)
cmake_minimum_required(VERSION 2.8)

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
find_package(ZLIB REQUIRED)
find_package(QJSON REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_INCLUDE_DIR} ${QJSON_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(paalternativelauncher_SRCS information.cpp advanceddialog.cpp bundle.cpp sha1.cpp patcher.cpp paalternativelauncher.cpp main.cpp)
qt4_automoc(${paalternativelauncher_SRCS})

QT4_ADD_RESOURCES(paalternativelauncher_RCC_SRCS paalternativelauncher.qrc)

add_executable(paalternativelauncher ${paalternativelauncher_SRCS} ${paalternativelauncher_RCC_SRCS})
if(WIN32)
	target_link_libraries(paalternativelauncher ${QT_QTNETWORK_LIBRARIES} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${ZLIB_LIBRARY} ${QJSON_LIBRARIES} -mwindows -lws2_32 -ljpeg -lpng -ltiff -llzma -lmng -lz -limm32 -llcms -lwinmm -lssl -lcrypto)
else()
	target_link_libraries(paalternativelauncher ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARIES} ${ZLIB_LIBRARY} ${QJSON_LIBRARIES})
endif()
install(TARGETS paalternativelauncher RUNTIME DESTINATION bin)
