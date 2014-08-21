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

	src/pointird/Unprojector/CalibrationDataFile.cpp
	src/pointird/Unprojector/CalibrationImageFile.cpp
	src/pointird/PointFilter/OffscreenFilter.cpp

	src/pointird/Capture/CaptureCV.cpp
	src/pointird/PointDetector/PointDetectorCV.cpp
	src/pointird/Unprojector/AutoUnprojectorCV.cpp
	src/pointird/PointOutput/DebugPointOutputCV.cpp
)

option( POINTIR_UNIXDOMAINSOCKET "Enable use of Unix Domain Sockets for point output and video stream" ${UNIX} )
option( POINTIR_UINPUT "Enable uinput API for multitouch device emulation output" ${LINUX} )
option( POINTIR_V4L2 "Enable Video4Linux2 API for video capture" ${LINUX} )
option( POINTIR_DBUS "Enable DBus controller" ON )

if( POINTIR_UINPUT )
	add_definitions( -DPOINTIR_UINPUT )
	list( APPEND POINTIR_SOURCES
		src/pointird/PointOutput/PointOutputUinput.cpp
	)
endif()

if( POINTIR_UNIXDOMAINSOCKET )
	add_definitions( -DPOINTIR_UNIXDOMAINSOCKET )
	list( APPEND POINTIR_SOURCES
		src/pointird/FrameOutput/UnixDomainSocketFrameOutput.cpp
		src/pointird/PointOutput/UnixDomainSocketPointOutput.cpp
	)
endif()

if( POINTIR_V4L2 )
	add_definitions( -DPOINTIR_V4L2 )
	list( APPEND POINTIR_SOURCES
		src/pointird/Capture/CaptureV4L2.cpp
	)
endif()

if( POINTIR_DBUS )
	add_definitions( -DPOINTIR_DBUS )
	list( APPEND POINTIR_SOURCES
		src/pointird/Controller/DBusController.cpp
	)
	find_package( DBus REQUIRED )
	include_directories( ${DBUS_INCLUDE_DIRS} )
	list( APPEND POINTIR_LIBRARIES ${DBUS_LIBRARIES} )
endif()

option( POINTIR_USE_SYSTEM_TCLAP "Use system TCLAP installation" OFF )
if( NOT POINTIR_USE_SYSTEM_TCLAP )
	include_directories( "${CMAKE_SOURCE_DIR}/external/tclap/include" )
endif()

find_package( OpenCV REQUIRED )
list( APPEND POINTIR_LIBRARIES ${OpenCV_LIBS} )

add_executable( ${POINTIR_EXECUTABLE_NAME} ${POINTIR_SOURCES} )
target_link_libraries( ${POINTIR_EXECUTABLE_NAME} ${POINTIR_LIBRARIES} )

if( WIN32 )
	target_link_libraries( ${POINTIR_EXECUTABLE_NAME} winmm imm32 version iphlpapi ws2_32 tiff lzma IlmImf z )
endif()

################################################################


################################################################
# Optional executables

option( POINTIR_BUILD_EXAMPLES "Builds example programs for testing" OFF )
option( POINTIR_BUILD_TOOLS "Builds tools for calibration etc" ON )

if( POINTIR_BUILD_EXAMPLES OR POINTIR_BUILD_TOOLS )
	option( POINTIR_USE_SYSTEM_SDL2 "Use system SDL2 installation" OFF )
	if( NOT POINTIR_USE_SYSTEM_SDL2 )
		add_subdirectory( "external/SDL2" )
		include_directories( "${CMAKE_SOURCE_DIR}/external/SDL2/include" )
		set( SDL2_LIBRARY SDL2-static )
		if( NEED_MAIN_WRAP )
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
	if( WIN32 )
		target_link_libraries( ${POINTIR_EXECUTABLE_NAME_EXAMPLE_SDL2} winmm dxguid imm32 version iphlpapi ws2_32 )
	endif()
endif()

if( POINTIR_BUILD_TOOLS )
	set( POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR "pointir_calibrate_SDL2" )
	add_executable( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} src/tool/SDL2Calibrator.cpp src/lodepng.cpp )
	target_link_libraries( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} ${SDL2_LIBRARY} ${DBUS_LIBRARIES} )
	if( WIN32 )
		target_link_libraries( ${POINTIR_EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} winmm dxguid imm32 version iphlpapi ws2_32 )
	endif()
endif()

################################################################
