cmake_minimum_required( VERSION 2.8 )
project( PointIR )


################################################################
# General project setup

if( CMAKE_SYSTEM_NAME STREQUAL Linux )
	set( LINUX ON )
endif()

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH} )

include_directories( "${CMAKE_SOURCE_DIR}/src" )
include_directories( "${CMAKE_SOURCE_DIR}/include" )

# configure compiler for c++11 etc.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11" )
endif()

################################################################


################################################################
# Daemon executable

set( POINTIR_EXECUTABLE_NAME "pointird" )

set( POINTIR_SOURCES
	src/lodepng.cpp
	src/pointird/main.cpp
	src/pointird/CaptureFactory.cpp
	src/pointird/OutputFactory.cpp
	src/pointird/ControllerFactory.cpp
	src/pointird/Processor.cpp

	src/pointird/TrackerFactory.cpp
	src/pointird/Tracker/Simple.cpp
	src/pointird/Tracker/Hungarian.cpp
	src/pointird/Tracker/ixoptimal.cpp

	src/pointird/Unprojector/CalibrationDataFile.cpp
	src/pointird/Unprojector/CalibrationImageFile.cpp
	src/pointird/PointFilter/OffscreenFilter.cpp
	src/pointird/PointFilter/LimitNumberFilter.cpp

	src/pointird/Capture/OpenCV.cpp
	src/pointird/PointDetector/OpenCV.cpp
	src/pointird/Unprojector/AutoOpenCV.cpp
	src/pointird/PointOutput/DebugOpenCV.cpp
)

option( POINTIR_PROCESSOR_BENCHMARK "Enable extra code to benchmark the Processor module - statistics are sent to stdout" OFF )
mark_as_advanced( POINTIR_PROCESSOR_BENCHMARK )
if( POINTIR_PROCESSOR_BENCHMARK )
	add_definitions( -DPOINTIR_PROCESSOR_BENCHMARK )
endif()

option( POINTIR_UNIXDOMAINSOCKET "Enable use of Unix Domain Sockets for point output and video stream" ${UNIX} )
option( POINTIR_UINPUT "Enable uinput API for multitouch device emulation output" ${LINUX} )
option( POINTIR_V4L2 "Enable Video4Linux2 API for video capture" ${LINUX} )
option( POINTIR_DBUS "Enable DBus controller" ON )
option( POINTIR_TUIO "Enable TUIO output module" ON )
option( POINTIR_WIN8TOUCHINJECTION "Enable Windows Touch Injection API" OFF )

if( POINTIR_UINPUT )
	add_definitions( -DPOINTIR_UINPUT )
	list( APPEND POINTIR_SOURCES
		src/pointird/PointOutput/Uinput.cpp
	)
endif()

if( POINTIR_UNIXDOMAINSOCKET )
	add_definitions( -DPOINTIR_UNIXDOMAINSOCKET )
	list( APPEND POINTIR_SOURCES
		src/pointird/FrameOutput/UnixDomainSocket.cpp
		src/pointird/PointOutput/UnixDomainSocket.cpp
	)
endif()

if( POINTIR_V4L2 )
	add_definitions( -DPOINTIR_V4L2 )
	list( APPEND POINTIR_SOURCES
		src/pointird/Capture/Video4Linux2.cpp
	)
endif()

if( POINTIR_DBUS )
	add_definitions( -DPOINTIR_DBUS )
	list( APPEND POINTIR_SOURCES
		src/pointird/Controller/DBus.cpp
	)
	find_package( DBus REQUIRED )
	include_directories( ${DBUS_INCLUDE_DIRS} )
	list( APPEND POINTIR_LIBRARIES ${DBUS_LIBRARIES} )
endif()

if( POINTIR_TUIO )
	add_definitions( -DPOINTIR_TUIO )
	list( APPEND POINTIR_SOURCES
		src/pointird/PointOutput/TUIO.cpp
	)
	find_package( liblo REQUIRED )
	include_directories( ${LIBLO_INCLUDE_DIRS} )
	add_definitions( ${LIBLO_DEFINITIONS} )
	list( APPEND POINTIR_LIBRARIES ${LIBLO_LIBRARIES} )
endif()

if( POINTIR_WIN8TOUCHINJECTION )
	add_definitions( -DPOINTIR_WIN8TOUCHINJECTION )
	list( APPEND POINTIR_SOURCES
		src/pointird/PointOutput/Win8TouchInjection.cpp
	)
	set( POINTIR_NEED_TRACKER ON )
endif()

option( POINTIR_USE_SYSTEM_TCLAP "Use system TCLAP installation" OFF )
if( NOT POINTIR_USE_SYSTEM_TCLAP )
	include_directories( "${CMAKE_SOURCE_DIR}/external/tclap/include" )
endif()

find_package( OpenCV REQUIRED )
list( APPEND POINTIR_LIBRARIES ${OpenCV_LIBS} )

find_package( Threads REQUIRED )
list( APPEND POINTIR_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} )

add_executable( ${POINTIR_EXECUTABLE_NAME} ${POINTIR_SOURCES} )
target_link_libraries( ${POINTIR_EXECUTABLE_NAME} ${POINTIR_LIBRARIES} )

# HACK:
if( WIN32 )
	target_link_libraries( ${POINTIR_EXECUTABLE_NAME} winmm imm32 version iphlpapi ws2_32 tiff lzma IlmImf z )
endif()

install( TARGETS ${POINTIR_EXECUTABLE_NAME} RUNTIME DESTINATION bin )
install( DIRECTORY include/ DESTINATION include )
if( UNIX )
	install( DIRECTORY etc/ DESTINATION share/PointIR/ExampleConfiguration/etc )
endif()
if( WIN32 )
	install( DIRECTORY etc.windows/ DESTINATION bin )
endif()
################################################################


################################################################
# Optional executables

option( POINTIR_BUILD_EXAMPLES "Build example programs for testing" OFF )
option( POINTIR_BUILD_TOOLS "Build tools for calibration etc" ON )

if( POINTIR_BUILD_EXAMPLES OR POINTIR_BUILD_TOOLS )
	option( POINTIR_USE_SYSTEM_SDL2 "Use system SDL2 installation" ON )
	if( NOT POINTIR_USE_SYSTEM_SDL2 )
		# TODO: find a way to disable install targets
		set( SDL_SHARED FALSE CACHE BOOL "Build a shared version of the library" )
		add_subdirectory( "external/SDL2" )
		include_directories( "${CMAKE_SOURCE_DIR}/external/SDL2/include" )
		set( SDL2_LIBRARY SDL2-static )
		# wrap the main function for those OSes
		if( ${CMAKE_SYSTEM_NAME} MATCHES "^Windows$|^Android$|^Darwin$" )
			list( APPEND ${SDL2_LIBRARY} SDL2main )
		endif()
	else()
		find_package( SDL2 REQUIRED )
		include_directories( ${SDL2_INCLUDE_DIR} )
	endif()
endif()

if( POINTIR_BUILD_EXAMPLES )
	set( POINTIR_EXECUTABLE_NAME_EXAMPLE_SDL2 "pointir_example_SDL2" )
	add_executable( ${POINTIR_EXECUTABLE_NAME_EXAMPLE_SDL2} src/example/SDL2.cpp )
	target_link_libraries( ${POINTIR_EXECUTABLE_NAME_EXAMPLE_SDL2} ${SDL2_LIBRARY} )
	# HACK:
	if( WIN32 )
		target_link_libraries( ${POINTIR_EXECUTABLE_NAME_EXAMPLE_SDL2} winmm dxguid imm32 version iphlpapi ws2_32 )
	endif()
endif()

if( POINTIR_BUILD_TOOLS )
	set( POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR "pointir_calibrate_SDL2" )
	add_executable( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} src/tool/SDL2Calibrator.cpp src/VideoSocketClient.cpp src/DBusClient.cpp src/lodepng.cpp )
	target_link_libraries( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} ${SDL2_LIBRARY} ${DBUS_LIBRARIES} )
	# HACK:
	if( WIN32 )
		target_link_libraries( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} winmm dxguid imm32 version iphlpapi ws2_32 )
	endif()
	install( TARGETS ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} RUNTIME DESTINATION bin )
endif()

################################################################


################################################################
# Packaging

#set( CPACK_PACKAGE_VERSION_MAJOR "0" )
#set( CPACK_PACKAGE_VERSION_MINOR "1" )
# TODO: this should be updated at build time - not only when configuring
# Only works on newer CMake versions:
#string( TIMESTAMP CPACK_PACKAGE_VERSION_PATCH "%Y%m%d" )

set( CPACK_PACKAGE_VERSION "0.1-snapshot" )

set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "PointIR - Captures a video stream, detects markers, and generates multi-touch input." )
set( CPACK_PACKAGE_CONTACT "Tobias Himmer <provisorisch@online.de>" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )

set( CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt" )

set( CPACK_SOURCE_IGNORE_FILES "/\\\\..*$;~$;/build.*/;${CPACK_SOURCE_IGNORE_FILES}" )

set( CPACK_DEBIAN_PACKAGE_DEPENDS "libdbus-1-3, libopencv-core2.4 (>= 2.4.1), libstdc++6 (>= 4.8.2)" )
if( POINTIR_TUIO )
	set( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, liblo7" )
endif()

set( CPACK_DEBIAN_PACKAGE_SECTION util )

if( WIN32 )
	if( "${CMAKE_SIZEOF_VOID_P}" EQUAL "8" )
		set( CPACK_SYSTEM_NAME win64 )
	endif()
	install( PROGRAMS "${CMAKE_FIND_ROOT_PATH}/bin/dbus-daemon.exe" DESTINATION bin)
endif()

include( CPack )

################################################################
