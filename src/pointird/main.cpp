/*
 * Copyright (C) 2014 Tobias Himmer <provisorisch@online.de>
 *
 * This file is part of PointIR.
 *
 * PointIR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PointIR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PointIR.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <vector>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef __unix__
	#include <signal.h>
	#include <sys/wait.h>
#else
	#include "PointOutput/Win8TouchInjection.hpp"
#endif

#include <tclap/CmdLine.h>

#include "OutputFactory.hpp"
#include "PointOutput/APointOutput.hpp"
#include "FrameOutput/AFrameOutput.hpp"

#include "CaptureFactory.hpp"
#include "Capture/ACapture.hpp"

#include "ControllerFactory.hpp"
#include "Controller/AController.hpp"

#include "PointDetector/OpenCV.hpp"

#include "Unprojector/AutoOpenCV.hpp"
#include "Unprojector/CalibrationDataFile.hpp"
#include "Unprojector/CalibrationImageFile.hpp"

#include "PointFilter/OffscreenFilter.hpp"
#include "PointFilter/LimitNumberFilter.hpp"
#include "PointFilter/Chain.hpp"

#include "Processor.hpp"

#include "exceptions.hpp"


static const char * notice =
"PointIR Daemon (compiled " __TIME__ ", " __DATE__ ")\n"
"This program processes a video stream to detect bright spots that are interpreted as \"touches\" for an emulated absolute pointing device (Touchscreen).\n"
"Copyright 2014 Tobias Himmer <provisorisch@online.de>";


static volatile bool running = true;

#ifdef __unix__
void shutdownHandler( int s )
{
	std::cerr << "Received signal " << s << " \"" << strsignal(s) << "\", shutting down!\n";
	running = false;
}
#endif

class CalibrationHook : public Processor::ACalibrationListener
{
public:
	void setBeginHook( const std::string & beginHook ) { this->beginHook = beginHook; }
	void setEndHook( const std::string & endHook ) { this->endHook = endHook; }
	const std::string & getBeginHook() { return this->beginHook; }
	const std::string & getEndHook() { return this->endHook; }

	static int system( const std::string & cmd )
	{
		int ret = ::system( cmd.c_str() );
		if( ret < 0 )
			throw SYSTEM_ERROR( errno, "system(\""+cmd+"\")");
#ifdef __unix__
		if( WIFEXITED(ret) )
			return WEXITSTATUS(ret);
		else
			throw RUNTIME_ERROR("\""+cmd+"\" terminated abnormaly");
#else
		return ret;
#endif
	}

	virtual void calibrationBegin() override
	{
		if( beginHook.empty() )
			return;
		std::cout << "CalibrationHook: Begin calibration - executing \"" << beginHook << "\"\n";
		std::cout << "--------\n";
		int ret = CalibrationHook::system( beginHook );
		std::cout << "--------\n";
		if( ret >= 0 )
			std::cout << "CalibrationHook: \"" << beginHook << "\" returned " << ret << "\n";
	}

	virtual void calibrationEnd( bool success ) override
	{
		if( endHook.empty() )
			return;
		std::string call = endHook + " " + (success?"1":"0");
		std::cout << "CalibrationHook: End calibration (" << (success?"success":"failure") << ") - executing \"" << call << "\"\n";
		std::cout << "--------\n";
		int ret = CalibrationHook::system( call );
		std::cout << "--------\n";
		if( ret >= 0 )
			std::cout << "CalibrationHook: \"" << endHook << "\" returned " << ret << "\n";
	}

private:
	std::string beginHook;
	std::string endHook;
};


int main( int argc, char ** argv )
{
	OutputFactory outputFactory;
	CaptureFactory captureFactory;
	ControllerFactory controllerFactory;

	std::string captureName;
	std::vector<std::string> outputNames;
	std::vector<std::string> controllerNames;
	CalibrationHook calibrationHook;


	////////////////////////////////////////////////////////////////
	// default daemon settings

	unsigned int pointLimit = 0;
	unsigned char detectorIntensityThreshold = 127;

	captureFactory.fps = 30.0f;
	captureFactory.width = 320;
	captureFactory.height = 240;

#ifdef POINTIR_V4L2
	captureName = "v4l2";
#else
	captureName = "cv";
#endif

#ifdef POINTIR_UINPUT
	outputNames.push_back( "uinput" );
#endif
#ifdef POINTIR_UNIXDOMAINSOCKET
	outputNames.push_back( "socket" );
#endif

#ifndef __unix__
	#if defined POINTIR_WIN8TOUCHINJECTION
	if( PointOutput::Win8TouchInjection::isAvailable() )
		outputNames.push_back( "win8" );
		#if defined POINTIR_TUIO
	else
		outputNames.push_back( "tuio" );
		#endif
	#elif defined POINTIR_TUIO
	outputNames.push_back( "tuio" );
	#endif
#endif

#ifdef POINTIR_DBUS
	controllerNames.push_back( "dbus" );
#endif

#ifdef __unix__
	captureFactory.deviceName = "/dev/video0";
	calibrationHook.setBeginHook( "/etc/PointIR/calibrationBeginHook" );
	calibrationHook.setEndHook( "/etc/PointIR/calibrationEndHook" );
#else
	calibrationHook.setBeginHook( "pointir_calibrationBeginHook.bat" );
	calibrationHook.setEndHook( "pointir_calibrationEndHook.bat" );
#endif

	////////////////////////////////////////////////////////////////


#ifdef __unix__
	////////////////////////////////////////////////////////////////
	// signal setup

	// install signal handler for SIGINT (interrupt from keyboard)
	struct sigaction signalHandler;
	signalHandler.sa_handler = shutdownHandler;
	sigemptyset( &signalHandler.sa_mask );
	signalHandler.sa_flags = 0;
	sigaction( SIGINT, &signalHandler, NULL );

	// ignore writes to detached pipes/sockets
	signal( SIGPIPE, SIG_IGN );

	////////////////////////////////////////////////////////////////
#endif

	////////////////////////////////////////////////////////////////
	// process command line options
	try
	{
		TCLAP::CmdLine cmd(
			notice,
			' ',     // delimiter
			__DATE__ // version
		);

		TCLAP::ValueArg<std::string> calibrationBeginHookArg(
			"", "calibBeginHook",
			"Script to execute when Calibration started.\nDefaults to \"" + calibrationHook.getBeginHook() + "\"",
			false, calibrationHook.getBeginHook(), "string", cmd );

		TCLAP::ValueArg<std::string> calibrationEndHookArg(
			"", "calibEndHook",
			"Script to execute when Calibration finished. An additional argument is appended on execution, indicating whether the calibration succeeded (1 for success, 0 for failure).\nDefaults to \"" + calibrationHook.getEndHook() + "\"",
			false, calibrationHook.getEndHook(), "string", cmd );

		TCLAP::ValueArg<std::string> deviceNameArg(
			"d", "device",
			"The camera device used to capture the video stream.\nDefaults to \"" + captureFactory.deviceName + "\"",
			false, captureFactory.deviceName, "string", cmd );

		TCLAP::ValueArg<int> widthArg(
			"", "width",
			"Width of captured video stream. If the device does not support the given resolution, the nearest possible value may be used.\nDefaults to " + std::to_string(captureFactory.width),
			false, captureFactory.width, "int", cmd );

		TCLAP::ValueArg<int> heigthArg(
			"", "height",
			"Height of captured video stream. If the device does not support the given resolution, the nearest possible value may be used.\nDefaults to " + std::to_string(captureFactory.height),
			false, captureFactory.height, "int", cmd );

		TCLAP::ValueArg<float> fpsArg(
			"", "fps",
			"Frame rate of captured video stream. If the device does not support the given frame rate, the nearest possible value may be used.\nDefaults to " + std::to_string(captureFactory.fps),
			false, captureFactory.fps, "float", cmd );

		TCLAP::ValueArg<int> pointLimitArg(
			"", "pointLimit",
			"Limit the number of points for the output. 0 to disable.\nDefaults to " + std::to_string(pointLimit),
			false, pointLimit, "int", cmd );

		TCLAP::ValueArg<int> detectorIntensityThresholdArg(
			"", "intensityThreshold",
			"The luminosity threshold used to detect points in the video capture.\nDefaults to " + std::to_string((unsigned int)detectorIntensityThreshold),
			false, detectorIntensityThreshold, "int", cmd );

		std::vector< std::string > availableTrackerNames = outputFactory.trackerFactory.getAvailableTrackerNames();
		TCLAP::ValuesConstraint<std::string> trackersArgConstraint( availableTrackerNames );
		TCLAP::ValueArg<std::string> trackerArg(
			"", "tracker",
			"The tracker used for outputs that need identifiiable contact points.\nDefaults to \"" + outputFactory.trackerFactory.getDefaultTrackerName() + "\"",
			false, outputFactory.trackerFactory.getDefaultTrackerName(), &trackersArgConstraint, cmd );

		std::vector< std::string > availableCaptureNames = captureFactory.getAvailableCaptureNames();
		TCLAP::ValuesConstraint<std::string> capturesArgConstraint( availableCaptureNames );
		TCLAP::ValueArg<std::string> captureArg(
			"", "capture",
			"The capture module used to retrieve the video stream.\nDefaults to \"" + captureName + "\"",
			false, captureName, &capturesArgConstraint, cmd );

		std::vector< std::string > availableOutputNames = outputFactory.getAvailableOutputNames();
		TCLAP::ValuesConstraint<std::string> outputsArgConstraint( availableOutputNames );
		std::string defaultOutputsAsArgument;
		for( std::string & output : outputNames )
			defaultOutputsAsArgument += "-o " + output + " ";
		if( defaultOutputsAsArgument.size() )
			defaultOutputsAsArgument.pop_back();
		TCLAP::MultiArg<std::string> outputsArg(
			"o",  "output",
			"Adds one or more output modules.\n"
#ifdef POINTIR_TUIO
			"For the TUIO protocol you can set the server address with the POINTIR_TUIO_ADDRESS environment variable, e.g. \"osc.udp://127.0.0.1:3331\".\n"
#endif
			"Specifying this will override the default (" + defaultOutputsAsArgument + ")",
			false, &outputsArgConstraint, cmd );

		std::vector< std::string > availableControllerNames = controllerFactory.getAvailableControllerNames();
		TCLAP::ValuesConstraint<std::string> controllersArgConstraint( availableControllerNames );
		std::string defaultControllersAsArgument;
		for( std::string & controller : controllerNames )
			defaultControllersAsArgument += "-x " + controller + " ";
		if( defaultControllersAsArgument.size() )
			defaultControllersAsArgument.pop_back();
		TCLAP::MultiArg<std::string> contollersArg(
			"",  "controller",
			"Adds one or more controller modules.\n"
#ifdef POINTIR_DBUS
			"For D-Bus you may have to set the DBUS_SYSTEM_BUS_ADDRESS environment variable to e.g. \"tcp:host=127.0.0.1,port=1234\".\n"
#endif
			"Specifying this will override the default (" + defaultControllersAsArgument + ")",
			false, &controllersArgConstraint, cmd );

		cmd.parse( argc, argv );

		calibrationHook.setBeginHook( calibrationBeginHookArg.getValue() );
		calibrationHook.setEndHook( calibrationEndHookArg.getValue() );

		outputFactory.trackerFactory.setDefaultTrackerName( trackerArg.getValue() );

		captureName = captureArg.getValue();

		captureFactory.deviceName = deviceNameArg.getValue();
		captureFactory.width = widthArg.getValue();
		captureFactory.height = heigthArg.getValue();
		captureFactory.fps = fpsArg.getValue();

		if( pointLimitArg.getValue() >= 0 )
			pointLimit = pointLimitArg.getValue();

		if( detectorIntensityThresholdArg.getValue() >= 0 )
			detectorIntensityThreshold = detectorIntensityThresholdArg.getValue();

		if( !outputsArg.getValue().empty() )
			outputNames = outputsArg.getValue();

		if( !contollersArg.getValue().empty() )
			controllerNames = contollersArg.getValue();
	}
	catch( TCLAP::ArgException & e )
	{
		std::cerr << "Command line error: \"" << e.error() << "\" for arg \"" << e.argId() << "\"\n";
		return 1;
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// build frame processor

	Capture::ACapture * capture = captureFactory.newCapture( captureName );
	if( !capture )
	{
		std::cerr << "Could not create capture \"" << captureName << "\"\n";
		return 1;
	}

	PointDetector::OpenCV detector;
//	detector.setBoundingFilterEnabled( true );
	detector.setIntensityThreshold( detectorIntensityThreshold );

	Unprojector::AutoOpenCV unprojector;
	Unprojector::CalibrationDataFile calibrationDataFile( unprojector );
	calibrationDataFile.load();

	PointFilter::Chain pointFilterChain;

	PointFilter::OffscreenFilter offscreenFilter;
	pointFilterChain.appendFilter( &offscreenFilter );

	PointFilter::LimitNumberFilter limitNumberFilter;
	if( pointLimit > 0 )
	{
		limitNumberFilter.setLimit( pointLimit );
		pointFilterChain.appendFilter( &limitNumberFilter );
	}

	Processor processor( *capture, detector, unprojector );
	processor.setPointFilter( &pointFilterChain );
	processor.addCalibrationListener( &calibrationHook );

	outputFactory.processor = &processor;
	for( std::string & outputName : outputNames )
	{
		PointOutput::APointOutput * pointOutput = outputFactory.newPointOutput( outputName );
		if( pointOutput )
			processor.addPointOutput( pointOutput );

		FrameOutput::AFrameOutput * frameOutput = outputFactory.newFrameOutput( outputName );
		if( frameOutput )
			processor.addFrameOutput( frameOutput );

		if( !pointOutput && ! frameOutput )
		{
			std::cerr << "Could not create output \"" << outputName << "\"\n";
			return 1;
		}
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// create processor controllers

	std::set< Controller::AController * > controllers;

	controllerFactory.processor = &processor;
	for( std::string & controllerName : controllerNames )
	{
		Controller::AController * controller = controllerFactory.newController( controllerName );
		if( !controller )
		{
			std::cerr << "Could not create controller \"" << controllerName << "\"\n";
			return 1;
		}
		controllers.insert( controller );
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// start the main loop and process each frame
	processor.start();
	while( running )
	{
		//TODO: instead of polling the "isProcessing" flag, maybe put controllers in threads and use some blocking mechanism?
		for( auto controller : controllers )
			controller->dispatch();
		if( processor.isProcessing() )
			processor.processFrame();
		else
			sleep( 1 );
	}
	processor.stop();
	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// cleanup

	for( auto controller : controllers )
		delete controller;

	delete capture;

	for( auto output : processor.getPointOutputs() )
	{
		processor.removePointOutput( output );
		delete output;
	}

	for( auto output : processor.getFrameOutputs() )
	{
		processor.removeFrameOutput( output );
		delete output;
	}

	////////////////////////////////////////////////////////////////


	return 0;
}
