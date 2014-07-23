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
#include <fstream>
#include <iterator>

#include <unistd.h>
#include <signal.h>

//#include <SDL.h>

#include <opencv2/highgui/highgui.hpp>

#include <tclap/CmdLine.h>

#include "Controller/DBusController.hpp"
#include "Capture/CaptureV4L2.hpp"
#include "PointDetector/PointDetectorCV.hpp"
#include "Unprojector/AutoUnprojectorCV.hpp"
#include "Unprojector/CalibrationDataFile.hpp"
#include "PointFilter/OffscreenFilter.hpp"
#include "PointFilter/PointFilterChain.hpp"
#include "PointOutput/DebugPointOutputCV.hpp"
#include "PointOutput/PointOutputUinput.hpp"
#include "Processor.hpp"


static const char * notice =
"PointIR Daemon (compiled " __TIME__ ", " __DATE__ " )\n"
"This program processes a video stream to detect bright spots that are interpreted as \"touches\" for an emulated absolute pointing device (Touchscreen).\n"
"Copyright © 2014 Tobias Himmer <provisorisch@online.de>\n"
"";

static volatile bool running = true;


void shutdownHandler( int s )
{
	std::cerr << "Received signal " << s << ", shutting down\n";
	running = false;
}


void calibrationBegin()
{
	std::cerr << "calibrationBegin\n";
}


void calibrationEnd()
{
	std::cerr << "calibrationEnd\n";
}


int main( int argc, char ** argv )
{
	std::string device = "/dev/video0";
	int width = 320;
	int height = 240;
	float fps = 30.0f;

	// install signal handler
	struct sigaction signalHandler;
	signalHandler.sa_handler = shutdownHandler;
	sigemptyset( &signalHandler.sa_mask );
	signalHandler.sa_flags = 0;
	sigaction( SIGINT, &signalHandler, NULL );

	try
	{
		TCLAP::CmdLine cmd(
			notice,
			' ',     // delimiter
			__DATE__ // version
		);

		TCLAP::ValueArg<std::string> deviceArg( "d",  "device", "Camera device",                       false, device, "string", cmd );
		TCLAP::ValueArg<int>          widthArg( "",   "width",  "Width of captured video stream",      false, width,  "int",    cmd );
		TCLAP::ValueArg<int>         heigthArg( "",   "height", "Height of captured video stream",     false, height, "int",    cmd );
		TCLAP::ValueArg<float>          fpsArg( "",   "fps",    "Frame rate of captured video stream", false, fps,    "float",  cmd );

		cmd.parse( argc, argv );

		device = deviceArg.getValue();
		width = widthArg.getValue();
		height = heigthArg.getValue();
		fps = fpsArg.getValue();
	}
	catch( TCLAP::ArgException & e )
	{
		std::cerr << "Command line error: " << e.error() << " for arg " << e.argId() << std::endl;
		return 1;
	}

/*
	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER );
	SDL_Window * window = SDL_CreateWindow(
		"PointIR",              // window title
		SDL_WINDOWPOS_CENTERED, // the x position of the window
		SDL_WINDOWPOS_CENTERED, // the y position of the window
		800, 600,               // window width and height
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL // flags
	);
	if( !window )
	{
		std::cerr << "Could not create SDL2 window: " << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_Renderer * renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_PRESENTVSYNC );
	if( !renderer )
	{
		std::cerr << "Could not create SDL2 renderer: " << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_RendererInfo info;
	if( SDL_GetRendererInfo( renderer, &info ) )
	{
		std::cerr << "Unable to retrieve SDL2 renderer information: " << SDL_GetError() << std::endl;
		return false;
	}
	std::cerr << "SDL2 renderer name: " << info.name << std::endl;
*/
	CaptureV4L2 capture( device, width, height, fps );
	capture.open();

	PointDetectorCV detector;
	detector.setBoundingFilterEnabled( true );

	AutoUnprojectorCV unprojector;
	CalibrationDataFile::load( unprojector );
/*
	cv::Mat calibrationImage( cv::Size( capture.getWidth(), capture.getHeight() ), CV_8UC1 );
	unprojector.generateCalibrationImage( calibrationImage.data, capture.getWidth(), capture.getHeight() );
	cv::imshow( "Calibration image", calibrationImage );
*/
	PointFilterChain pointFilterChain;
	OffscreenFilter offscreenFilter;
	pointFilterChain.appendFilter( &offscreenFilter );

	Processor processor( capture, detector, unprojector );
	processor.setPointFilter( &pointFilterChain );
	processor.setCalibrationBeginCallback( calibrationBegin );
	processor.setCalibrationEndCallback( calibrationEnd );

//	DebugPointOutputCV debugPointOutputCV( &processor );
	PointOutputUinput pointOutputUinput;
//	processor.addOutput( &debugPointOutputCV );
	processor.addOutput( &pointOutputUinput );

	DBusController controller( processor );

	processor.start();

	while( running )
	{
		//TODO: use "select" to block until a controller reenables processing - for now keep polling
		controller.dispatch();
		if( processor.isProcessing() )
			processor.processFrame();
		else
			sleep( 1 );
/*
		cv::Mat unprojectedFrame( cv::Size( processor.getFrameWidth(), processor.getFrameHeight() ), CV_8UC1 );
		memcpy( unprojectedFrame.data, processor.getProcessedFrame().data(), processor.getProcessedFrame().size() );
		unprojector.unproject( unprojectedFrame.data, processor.getFrameWidth(), processor.getFrameHeight() );
		cv::imshow( "Unprojected frame", unprojectedFrame );
*/
/*
		switch( cv::waitKey( 10 ) ) // wait 10ms for key press
		{
		case 27:
			std::cout << "exiting" << std::endl;
			running = false;
			break;
		case 'c':
			std::cout << "calibrating ... ";
			if( unprojector.calibrate( processor.getProcessedFrame().data(), processor.getFrameWidth(), processor.getFrameHeight() ) )
				std::cout << "ok" << std::endl;
			else
				std::cout << "failed" << std::endl;
			break;
		case 'o':
			processor.setOutputEnabled( !processor.isOutputEnabled() );
			if( processor.isOutputEnabled() )
				std::cout << "enabled output\n";
			else
				std::cout << "disabled output\n";
			break;
		case 's':
			{
				std::cout << "saving calibration data" << std::endl;
				std::ofstream file( "/tmp/PointIR.calib", std::ios::out | std::ofstream::binary );
				std::vector< uint8_t > rawData = unprojector.getRawCalibrationData();
				std::copy( rawData.begin(), rawData.end(), std::ostreambuf_iterator< char >(file) );
			}
			break;
		case 'l':
			{
				std::cout << "loading calibration data ... ";
				std::ifstream file( "/tmp/PointIR.calib", std::ios::in | std::ifstream::binary );
				std::vector< uint8_t > rawData;
				std::copy( std::istreambuf_iterator< char >( file ), std::istreambuf_iterator< char >(), std::back_inserter(rawData) );
				if( unprojector.setRawCalibrationData( rawData ) )
					std::cout << "ok" << std::endl;
				else
					std::cout << "failed" << std::endl;
			}
			break;
		}
*/
	}

/*
	SDL_Texture * texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, capture.getWidth(), capture.getHeight() );
	std::vector<unsigned char> converted;
	converted.resize( capture.getWidth() * capture.getHeight() * 3 );

	unsigned int lastTime = 0, currentTime;
	bool running = true;
	while( running )
	{
		currentTime = SDL_GetTicks();
		float delta = (currentTime - lastTime) / 1000.0;
		lastTime = currentTime;

		SDL_Event event;
		while( SDL_PollEvent(&event) )
		{
			switch( event.type )
			{
				case SDL_QUIT:
				{
					running = false;
					break;
				}
				case SDL_KEYDOWN:
				{
					SDL_Keycode key = event.key.keysym.sym;
					switch( key )
					{
					case SDLK_ESCAPE:
						running = false;
						break;
					default:
						break;
					}
					break;
				}
				default:
					break;
			}
		}


		capture.advanceFrame();
		capture.frameAsGrayscale( data.data(), data.size() );
		cv::Mat imgGray( capture.getHeight(), capture.getWidth(), CV_8UC1, data.data() );

		cv::Mat imgThresholded;
		cv::inRange( imgGray, cv::Scalar( iLowV ), cv::Scalar( iHighV ), imgThresholded ); //Threshold the image


		SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
		SDL_RenderClear( renderer );

		uint8_t* pixelPtr = (uint8_t*)imgThresholded.data;
		for(int i = 0; i < imgThresholded.rows; i++)
		{
			for(int j = 0; j < imgThresholded.cols; j++)
			{
				unsigned char gray = pixelPtr[ i * imgThresholded.cols + j ];
				converted[ i * imgThresholded.cols * 3 + j * 3 + 0 ] = gray;
				converted[ i * imgThresholded.cols * 3 + j * 3 + 1 ] = gray;
				converted[ i * imgThresholded.cols * 3 + j * 3 + 2 ] = gray;
			}
		}
		SDL_UpdateTexture( texture, NULL, converted.data(), capture.getWidth() * 3 );
		SDL_RenderCopy( renderer, texture, NULL, NULL );

		SDL_RenderPresent( renderer );

		std::cerr << "Took " << delta << " seconds\r";
	}
*/

	processor.stop();
	capture.close();
/*
	SDL_DestroyTexture( texture );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();
*/
	return 0;
}
