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

#include <SDL.h>
#include <dbus/dbus.h>

#include <lodepng.h>

#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>


#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


constexpr static const char * dBusName = "PointIR.Calibrator";

constexpr static const char * dBusControllerName = "PointIR.Controller";
constexpr static const char * dBusControllerObject = "/PointIR/Controller";


static std::vector< uint8_t > loadCalibrationImage( unsigned int width, unsigned int height, DBusConnection * dBusConnection )
{
	DBusMessage * msg = dbus_message_new_method_call(
		dBusControllerName,               // target for the method call
		dBusControllerObject,             // object to call on
		"PointIR.Controller.Unprojector", // interface to call on
		"generateCalibrationImageFile"    // method name
	);
	if( !msg )
		throw RUNTIME_ERROR( "dbus_message_new_method_call failed" );

	// append arguments
	DBusMessageIter args;
	dbus_message_iter_init_append( msg, &args );

	DBusBasicValue value;
	value.u32 = width;
	if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, &value ) )
		throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );
	value.u32 = height;
	if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, &value ) )
		throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );

	// send message and get a handle for a reply
	DBusError dBusError;
	dbus_error_init( &dBusError );
	DBusMessage * reply = dbus_connection_send_with_reply_and_block( dBusConnection, msg, -1, &dBusError ); // -1 is default timeout
	if( !reply )
		throw RUNTIME_ERROR( "dbus_connection_send_with_reply_and_block failed: " + std::string(dBusError.message) );

	// handle arguments in reply
	if( !dbus_message_iter_init( reply, &args ) )
		throw RUNTIME_ERROR( "Expected one argument in reply" );
	if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_STRING )
		throw RUNTIME_ERROR( "Expected argument of type string" );
	dbus_message_iter_get_basic( &args, &value );
	std::string filename( value.str );

	std::vector< uint8_t > image;
	unsigned int loadedWidth = 0;
	unsigned int loadedHeight = 0;
	unsigned int error = lodepng::decode( image, loadedWidth, loadedHeight, filename, LodePNGColorType::LCT_GREY, 8 );
	if( error )
		throw RUNTIME_ERROR( "LodePNG encode error " + std::to_string(error) + ": " + lodepng_error_text(error) );
	if( loadedWidth != width || loadedHeight != height )
		throw RUNTIME_ERROR( "PointIR daemon generated a calibration image of different size" );

	// free messages
	dbus_message_unref(msg);
	dbus_message_unref(reply);

	return image;
}


static bool dBusGetBool( DBusConnection * dBusConnection, const std::string & interface, const std::string & method )
{
	DBusMessage * msg = dbus_message_new_method_call(
		dBusControllerName,    // target for the method call
		dBusControllerObject, // object to call on
		interface.c_str(),   // interface to call on
		method.c_str()      // method name
	);
	if( !msg )
		throw RUNTIME_ERROR( "dbus_message_new_method_call failed" );

	// append arguments
	DBusMessageIter args;
	dbus_message_iter_init_append( msg, &args );

	// send message and get a handle for a reply
	DBusError dBusError;
	dbus_error_init( &dBusError );
	DBusMessage * reply = dbus_connection_send_with_reply_and_block( dBusConnection, msg, -1, &dBusError ); // -1 is default timeout
	if( !reply )
		throw RUNTIME_ERROR( "dbus_connection_send_with_reply_and_block failed: " + std::string(dBusError.message) );

	// handle arguments in reply
	DBusBasicValue value;
	if( !dbus_message_iter_init( reply, &args ) )
		throw RUNTIME_ERROR( "Expected one argument in reply" );
	if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_BOOLEAN )
		throw RUNTIME_ERROR( "Expected argument of type bool" );
	dbus_message_iter_get_basic( &args, &value );
	bool result = value.bool_val;

	// free messages
	dbus_message_unref(msg);
	dbus_message_unref(reply);

	return result;
}


int main( int argc, char ** argv )
{
	////////////////////////////////////////////////////////////////
	// DBus init

	DBusError dBusError;
	dbus_error_init( &dBusError );
	DBusConnection * dBusConnection = nullptr;
	try
	{
		dBusConnection = dbus_bus_get( DBUS_BUS_SYSTEM, &(dBusError) );
		if( dbus_error_is_set( &dBusError ) )
			throw RUNTIME_ERROR( "Connection Error: " + std::string(dBusError.message) );
		if( !dBusConnection )
			throw RUNTIME_ERROR( "Could not connect to system DBus" );

		int ret = dbus_bus_request_name(
			dBusConnection,
			dBusName,
			0,
			&dBusError
		);
		if( dbus_error_is_set( &dBusError ) )
			throw RUNTIME_ERROR( "Name Error: " + std::string(dBusError.message) );
		if( DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret )
			throw RUNTIME_ERROR( "Could not request name on system DBus" );
	} catch( ... )
	{
		// uninitialise dbus and rethrow exception
		if( dBusConnection )
			dbus_connection_unref( dBusConnection );
		dbus_error_free( &dBusError );
		throw;
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// SDL2 init

	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS );

	SDL_Window * window = SDL_CreateWindow(
		"PointIR Calibration (SDL2)",  // title
		SDL_WINDOWPOS_UNDEFINED,       // position x
		SDL_WINDOWPOS_UNDEFINED,       // position y
		640,                           // width
		480,                           // height
		SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE // flags
	);
	if( !window )
	{
		printf( "Could not create window: %s\n", SDL_GetError() );
		return 1;
	}

	SDL_Renderer * renderer = SDL_CreateRenderer(
		window,                                              // window handle
		-1,                                                  // device index: -1 for first match
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC // flags
	);
	if( !renderer )
	{
		printf( "Could not create renderer: %s\n", SDL_GetError() );
		return 1;
	}

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// perform calibration

	// retrieve calibration image of the same size as our fullscreen window
	int w = 0, h = 0;
	SDL_GetWindowSize( window, &w, &h );
	std::vector< uint8_t > calibrationImageGreyscale = loadCalibrationImage( w, h, dBusConnection );

	// SDL only supports 3/4 component images !? need to convert here
	std::vector< uint8_t > calibrationImage( calibrationImageGreyscale.size() * 3 );
	for( unsigned int i = 0; i < calibrationImageGreyscale.size(); i++ )
	{
		calibrationImage[i*3] = calibrationImageGreyscale[i];
		calibrationImage[i*3+1] = calibrationImageGreyscale[i];
		calibrationImage[i*3+2] = calibrationImageGreyscale[i];
	}

	// upload converted image to texture
	SDL_Texture * calibrationImageTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, w, h );
	SDL_UpdateTexture( calibrationImageTexture, nullptr, calibrationImage.data(), w * 3 );

	// display texture
	SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
	SDL_RenderClear( renderer );
	SDL_RenderCopy( renderer, calibrationImageTexture, nullptr, nullptr );
	SDL_RenderPresent( renderer );

	// try to calibrate a few times
	constexpr static const unsigned int maxTries = 3;
	bool success = false;
	for( unsigned int i = 1; i <= maxTries; i++ )
	{
		// calibration image probably takes some time until it hits the daemon - wait a bit to be sure
		sleep(1);
		// this will trigger a calibration attempt
		success = dBusGetBool( dBusConnection, "PointIR.Controller.Processor", "calibrate" );
		if( success )
		{
			std::cout << "Calibration attempt " << i << " of " << maxTries << ": Succeeded :)\n";
			break;
		}
		else
		{
			std::cout << "Calibration attempt " << i << " of " << maxTries << ": Failed :(\n";
		}
	}

	// if calibration went well, make the daemon save the calibration data
	if( success )
		dBusGetBool( dBusConnection, "PointIR.Controller.Unprojector", "saveCalibrationData" );

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// SDL2 shutdown
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();
	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// DBus shutdown
	if( dBusConnection )
		dbus_connection_unref( dBusConnection );
	dbus_error_free( &dBusError );
	////////////////////////////////////////////////////////////////

	return 0;
}
