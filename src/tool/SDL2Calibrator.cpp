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
#include <string.h>
#include <poll.h>
#include <malloc.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>
#include <system_error>


#define SYSTEM_ERROR( errornumber, whattext ) \
	std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


//HACK: flexible array members (char array[]) are part of C99 but not C++11 and below - however they work with g++, so just disable the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t data[];
} PointIR_Frame;
#pragma GCC diagnostic pop


constexpr static const char * dBusName = "PointIR.Calibrator";
constexpr static const char * dBusControllerName = "PointIR.Controller";
constexpr static const char * dBusControllerObject = "/PointIR/Controller";

constexpr static const char * videoSocketName = "/tmp/PointIR.video.socket";


static SDL_Texture * receiveFrame( SDL_Renderer * renderer, int socketFD )
{
	static SDL_Texture * videoTexture = nullptr;
	static size_t bufferSize = sizeof(PointIR_Frame);
	static PointIR_Frame * buffer = (PointIR_Frame *) calloc( 1, bufferSize );

	ssize_t received;

	// peek at the next packet - return last frame if nothing received
	PointIR_Frame peek;
	received = recv( socketFD, &peek, sizeof(peek), MSG_PEEK );
	if( -1 == received )
	{
		if( EAGAIN == errno || EWOULDBLOCK == errno )
			return videoTexture;
		else
			throw SYSTEM_ERROR( errno, "recv" );
	}
	if( sizeof(peek) != received )
		throw RUNTIME_ERROR( "too few data received" );

	// resize packet buffer and texture if needed
	if( peek.width != buffer->width || peek.height != buffer->height )
	{
		buffer->width = peek.width;
		buffer->height = peek.height;
		if( videoTexture )
			SDL_DestroyTexture( videoTexture );
		videoTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, buffer->width, buffer->height );
		bufferSize = sizeof(PointIR_Frame) + buffer->width * buffer->height;
		buffer = (PointIR_Frame *) realloc( buffer, bufferSize );
		if( !buffer )
			throw SYSTEM_ERROR( errno, "realloc" );
		std::cerr << std::string(__PRETTY_FUNCTION__) << ": resized to "<<buffer->width<<"x"<< buffer->height<<"\n";
	}

	// receive the packet
	received = recv( socketFD, buffer, bufferSize, 0 );
	if( -1 == received )
		throw SYSTEM_ERROR( errno, "recv" );
	if( bufferSize != (size_t)received )
		throw RUNTIME_ERROR( "too few data received" );

	// SDL only supports 3/4 component images !? need to convert here
	size_t frameSize = buffer->width * buffer->height;
	std::vector< uint8_t > frame3( frameSize * 3 );
	for( unsigned int i = 0; i < frameSize; i++ )
	{
		frame3[i*3] = buffer->data[i];
		frame3[i*3+1] = buffer->data[i];
		frame3[i*3+2] = buffer->data[i];
	}

	SDL_UpdateTexture( videoTexture, nullptr, frame3.data(), buffer->width * 3 );
	return videoTexture;
}


static SDL_Texture * loadCalibrationImage( SDL_Renderer * renderer, unsigned int width, unsigned int height, DBusConnection * dBusConnection )
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

	// SDL only supports 3/4 component images !? need to convert here
	std::vector< uint8_t > image3( image.size() * 3 );
	for( unsigned int i = 0; i < image.size(); i++ )
	{
		image3[i*3] = image[i];
		image3[i*3+1] = image[i];
		image3[i*3+2] = image[i];
	}

	// upload converted image to texture
	SDL_Texture * calibrationImageTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, width, height );
	SDL_UpdateTexture( calibrationImageTexture, nullptr, image3.data(), width * 3 );

	return calibrationImageTexture;
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
	// video socket init

	int videoFD = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
	if( -1 == videoFD )
		throw SYSTEM_ERROR( errno, "socket" );

	// set local socket nonblocking
	int flags = fcntl( videoFD, F_GETFL, 0 );
	if( -1 == flags )
		throw SYSTEM_ERROR( errno, "fcntl" );
	if( -1 == fcntl( videoFD, F_SETFL, flags | O_NONBLOCK ) )
		throw SYSTEM_ERROR( errno, "fcntl" );

	struct sockaddr_un remoteAddr;
	remoteAddr.sun_family = AF_UNIX;
	strcpy( remoteAddr.sun_path, videoSocketName );
	size_t len = strlen(remoteAddr.sun_path) + sizeof(remoteAddr.sun_family);
	if( -1 == connect( videoFD, (struct sockaddr *)&remoteAddr, len ) )
		throw SYSTEM_ERROR( errno, "connect" );

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
	// the main loop

	// retrieve calibration image of the same size as our fullscreen window
	int w = 0, h = 0;
	SDL_GetWindowSize( window, &w, &h );
	SDL_Texture * calibrationImageTexture = loadCalibrationImage( renderer, w, h, dBusConnection );

	struct Touch
	{
		SDL_Point point;
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	std::map< int64_t, Touch > touches;

	bool closeRequested = false;
	bool calibrate = false;
	SDL_Texture * videoTexture;
	while( !closeRequested )
	{
		SDL_Event e;
		while( SDL_PollEvent(&e) )
		{
			switch( e.type )
			{
				case SDL_QUIT:
					closeRequested = true;
					break;
				case SDL_KEYDOWN:
					switch( e.key.keysym.sym )
					{
					case SDLK_ESCAPE:
						closeRequested = true;
						break;
					case SDLK_SPACE:
						calibrate = true;
						break;
					case SDLK_RETURN:
						if( SDL_GetModState() & KMOD_ALT )
						{
							static Uint32 lastFullscreenFlags = SDL_WINDOW_FULLSCREEN_DESKTOP;
							Uint32 fullscreenFlags = SDL_GetWindowFlags( window ) & ( SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP );
							if( fullscreenFlags )
							{
								lastFullscreenFlags = fullscreenFlags;
								fullscreenFlags = 0;
							}
							else
							{
								fullscreenFlags = lastFullscreenFlags;
							}
							SDL_SetWindowFullscreen( window, fullscreenFlags );
						}
						break;
					}
					break;
				case SDL_FINGERDOWN:
					{
						Touch t;
						t.point.x = e.tfinger.x;
						t.point.y = e.tfinger.y;
						t.r = rand() % 128 + 127;
						t.g = rand() % 128 + 127;
						t.b = rand() % 128 + 127;
						touches[ e.tfinger.fingerId ] = t;
					}
					break;
				case SDL_FINGERUP:
					touches.erase( e.tfinger.fingerId );
					break;
				case SDL_FINGERMOTION:
					{
						auto i = touches.find( e.tfinger.fingerId );
						if( i != touches.end() )
						{
							Touch & t = i->second;
							t.point.x = e.tfinger.x;
							t.point.y = e.tfinger.y;
						}
					}
					break;
			}
		}

		SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
		SDL_RenderClear( renderer );

		if( calibrate )
		{
			calibrate = false;
			SDL_RenderCopy( renderer, calibrationImageTexture, nullptr, nullptr );
			SDL_RenderPresent( renderer );
			sleep(1);
			if( dBusGetBool( dBusConnection, "PointIR.Controller.Processor", "calibrate" ) )
			{
				std::cout << "Calibration succeeded :)\n";
				dBusGetBool( dBusConnection, "PointIR.Controller.Unprojector", "saveCalibrationData" );
			}
			else
			{
				std::cout << "Calibration failed :(\n";
			}
		}

		videoTexture = receiveFrame( renderer, videoFD );
		if( videoTexture )
			SDL_RenderCopy( renderer, videoTexture, nullptr, nullptr );

		for( auto & p : touches )
		{
			const Touch & touch = p.second;
			SDL_SetRenderDrawColor( renderer, touch.r, touch.g, touch.b, 0xff );
			SDL_RenderDrawLine( renderer, 0, touch.point.y, w, touch.point.y );
			SDL_RenderDrawLine( renderer, touch.point.x, 0, touch.point.x, h );
		}

		SDL_RenderPresent( renderer );
	}
	if( videoTexture )
		SDL_DestroyTexture( videoTexture );

	////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////
	// SDL2 shutdown
	SDL_DestroyTexture( calibrationImageTexture );
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
