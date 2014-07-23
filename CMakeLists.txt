cmake_minimum_required( VERSION 2.8 )
project( PointIR )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH} )

set( EXECUTABLE_NAME "pointird" )
add_executable( ${EXECUTABLE_NAME}
	src/lodepng.cpp
	src/pointird/main.cpp
	src/pointird/Processor.cpp
	src/pointird/Capture/CaptureV4L2.cpp
	src/pointird/PointDetector/PointDetectorCV.cpp
	src/pointird/Unprojector/AutoUnprojectorCV.cpp
	src/pointird/Unprojector/CalibrationDataFile.cpp
	src/pointird/Unprojector/CalibrationImageFile.cpp
	src/pointird/PointOutput/DebugPointOutputCV.cpp
	src/pointird/PointOutput/PointOutputUinput.cpp
	src/pointird/Controller/DBusController.cpp
	src/pointird/PointFilter/OffscreenFilter.cpp
)

include_directories( "${CMAKE_SOURCE_DIR}/src" )

# configure compiler for c++11 etc.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11" )
endif()

option( USE_SYSTEM_TCLAP "Use system TCLAP installation" OFF )
if( NOT USE_SYSTEM_TCLAP )
	include_directories( "${CMAKE_SOURCE_DIR}/external/tclap/include" )
endif()

find_package( OpenCV REQUIRED )
target_link_libraries( ${EXECUTABLE_NAME} ${OpenCV_LIBS} )

find_package( DBus REQUIRED )
include_directories( ${DBUS_INCLUDE_DIRS} )
target_link_libraries( ${EXECUTABLE_NAME} ${DBUS_LIBRARIES} )


option( POINTIR_BUILD_EXAMPLES "Builds example programs for testing" OFF )
option( POINTIR_BUILD_TOOLS "Builds tools for calibration etc" ON )


if( POINTIR_BUILD_EXAMPLES OR POINTIR_BUILD_TOOLS )
	option( USE_SYSTEM_SDL2 "Use system SDL2 installation" OFF )
	if( NOT USE_SYSTEM_SDL2 )
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
	set( EXECUTABLE_NAME_EXAMPLE_SDL2 "pointir_example_SDL2" )
	add_executable( ${EXECUTABLE_NAME_EXAMPLE_SDL2} src/example/SDL2.cpp )
	target_link_libraries( ${EXECUTABLE_NAME_EXAMPLE_SDL2} ${SDL2_LIBRARY} )
endif()


if( POINTIR_BUILD_TOOLS )
	set( EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR "pointir_calibrate_SDL2" )
	add_executable( ${EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} src/tool/SDL2Calibrator.cpp src/lodepng.cpp )
	target_link_libraries( ${EXECUTABLE_NAME_TOOL_SDL2CALIBRATOR} ${SDL2_LIBRARY} ${DBUS_LIBRARIES} )
endif()
