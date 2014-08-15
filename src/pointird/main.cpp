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

#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <tclap/CmdLine.h>

#include "Controller/DBusController.hpp"
#include "Capture/CaptureV4L2.hpp"
#include "PointDetector/PointDetectorCV.hpp"
#include "Unprojector/AutoUnprojectorCV.hpp"
#include "Unprojector/CalibrationDataFile.hpp"
#include "PointFilter/OffscreenFilter.hpp"
#include "PointFilter/PointFilterChain.hpp"
#include "PointOutput/APointOutput.hpp"
#include "FrameOutput/AFrameOutput.hpp"
#include "Processor.hpp"
#include "OutputFactory.hpp"


static const char * notice =
"PointIR Daemon (compiled " __TIME__ ", " __DATE__ ")\n"
"This program processes a video stream to detect bright spots that are interpreted as \"touches\" for an emulated absolute pointing device (Touchscreen).\n"
"Copyright © 2014 Tobias Himmer <provisorisch@online.de>";


static volatile bool running = true;


void shutdownHandler( int s )
{
	std::cerr << "Received signal " << s << " \"" << strsignal(s) << "\", shutting down!\n";
	running = false;
}


class CalibrationHook : public Processor::ACalibrationListener
{
public:
	CalibrationHook( const std::string & beginHook, const std::string & endHook ) :
		beginHook(beginHook), endHook(endHook)
	{}

	void setBeginHook( const std::string & beginHook ) { this->beginHook = beginHook; }
	void setEndHook( const std::string & endHook ) { this->endHook = endHook; }
	const std::string & getBeginHook() { return this->beginHook; }
	const std::string & getEndHook() { return this->endHook; }

	virtual void calibrationBegin() override
	{
		std::cout << "CalibrationHook: Begin calibration - executing \"" << beginHook << "\"\n";
		std::cout << "--------\n";
		int ret = system( beginHook.c_str() );
		std::cout << "--------\n";
		std::cout << "CalibrationHook: \"" << beginHook << "\" returned " << ret << "\n";
	}

	virtual void calibrationEnd( bool success ) override
	{
		std::string call = endHook + " " + (success?"1":"0");
		std::cout << "CalibrationHook: End calibration (" << (success?"success":"failure") << ") - executing \"" << call << "\"\n";
		std::cout << "--------\n";
		int ret = system( call.c_str() );
		std::cout << "--------\n";
		std::cout << "CalibrationHook: \"" << endHook << "\" returned " << ret << "\n";
	}

private:
	std::string beginHook;
	std::string endHook;
};


int main( int argc, char ** argv )
{
	////////////////////////////////////////////////////////////////
	// default daemon settings
	std::string device = "/dev/video0";
	int width = 320;
	int height = 240;
	float fps = 30.0f;
	std::vector<std::string> outputNames( {"uinput", "socket"} );
	CalibrationHook calibrationHook( "/etc/PointIR/calibrationBeginHook", "/etc/PointIR/calibrationEndHook" );
	////////////////////////////////////////////////////////////////


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


	////////////////////////////////////////////////////////////////
	// process command line options

	// create a list of available outputs that can be added later
	OutputFactory outputFactory;
	std::vector< std::string > availableOutputNames = outputFactory.getAvailableOutputs();
	TCLAP::ValuesConstraint<std::string> outputsArgConstraint( availableOutputNames );

	// default list
	std::string defaultOutputsAsArgument;
	for( std::string & output : outputNames )
		defaultOutputsAsArgument += "-o " + output + " ";
	defaultOutputsAsArgument.pop_back();

	try
	{
		TCLAP::CmdLine cmd(
			notice,
			' ',     // delimiter
			__DATE__ // version
		);

		TCLAP::ValueArg<std::string> calibrationBeginHookArg(
			"", "beginCalibHook",
			"Script to execute when Calibration started.\nDefaults to \"" + calibrationHook.getBeginHook() + "\"",
			false, calibrationHook.getBeginHook(), "string", cmd );

		TCLAP::ValueArg<std::string> calibrationEndHookArg(
			"", "endCalibHook",
			"Script to execute when Calibration finished.\nDefaults to \"" + calibrationHook.getEndHook() + "\"",
			false, calibrationHook.getEndHook(), "string", cmd );

		TCLAP::ValueArg<std::string> deviceArg(
			"d", "device",
			"The camera device used to capture the video stream.\nDefaults to \"" + device + "\"",
			false, device, "string", cmd );

		TCLAP::ValueArg<int> widthArg(
			"", "width",
			"Width of captured video stream. If the device does not support the given resolution, the nearest possible value is used.\nDefaults to " + std::to_string(width),
			false, width, "int", cmd );

		TCLAP::ValueArg<int> heigthArg(
			"", "height",
			"Height of captured video stream. If the device does not support the given resolution, the nearest possible value is used.\nDefaults to " + std::to_string(height),
			false, height, "int", cmd );

		TCLAP::ValueArg<float> fpsArg(
			"", "fps",
			"Frame rate of captured video stream. If the device does not support the given frame rate, the nearest possible value is used.\nDefaults to " + std::to_string(fps),
			false, fps, "float", cmd );

		TCLAP::MultiArg<std::string> outputsArg(
			"o",  "output",
			"Adds one or more output modules.\nSpecifying this will override the default (" + defaultOutputsAsArgument + ")",
			false, &outputsArgConstraint, cmd );

		cmd.parse( argc, argv );

		calibrationHook.setBeginHook( calibrationBeginHookArg.getValue() );
		calibrationHook.setEndHook( calibrationEndHookArg.getValue() );
		device = deviceArg.getValue();
		width = widthArg.getValue();
		height = heigthArg.getValue();
		fps = fpsArg.getValue();
		if( !outputsArg.getValue().empty() )
			outputNames = outputsArg.getValue();
	}
	catch( TCLAP::ArgException & e )
	{
		std::cerr << "Command line error: \"" << e.error() << "\" for arg \"" << e.argId() << "\"\n";
		return 1;
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// build frame processor

	CaptureV4L2 capture( device, width, height, fps );

	PointDetectorCV detector;
	detector.setBoundingFilterEnabled( true );

	AutoUnprojectorCV unprojector;
	CalibrationDataFile::load( unprojector );

	PointFilterChain pointFilterChain;

	OffscreenFilter offscreenFilter;
	pointFilterChain.appendFilter( &offscreenFilter );

	Processor processor( capture, detector, unprojector );
	processor.setPointFilter( &pointFilterChain );
	processor.addCalibrationListener( &calibrationHook );

	outputFactory.setProcessor( &processor );
	for( std::string & outputName : outputNames )
	{
		APointOutput * pointOutput = outputFactory.newPointOutput( outputName );
		if( pointOutput )
			processor.addPointOutput( pointOutput );

		AFrameOutput * frameOutput = outputFactory.newFrameOutput( outputName );
		if( frameOutput )
			processor.addFrameOutput( frameOutput );
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// create processor controllers
	DBusController controller( processor );
	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// start the main loop and process each frame
	processor.start();
	while( running )
	{
		//TODO: instead of polling the "isProcessing" flag, maybe put controllers in threads and use some blocking mechanism?
		controller.dispatch();
		if( processor.isProcessing() )
			processor.processFrame();
		else
			sleep( 1 );
	}
	processor.stop();
	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// cleanup

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
