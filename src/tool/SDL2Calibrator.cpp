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

#include "VideoSocketClient.hpp"
#include "DBusClient.hpp"
#include "lodepng.h"

#include <SDL.h>

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>
#include <system_error>


#define SYSTEM_ERROR( errornumber, whattext ) \
	std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


static PointIR::VideoSocketClient video;

static SDL_Texture * receiveFrame( SDL_Renderer * renderer )
{
	static SDL_Texture * videoTexture = nullptr;
	static PointIR::Frame frame;

	if( !video.receiveFrame( frame ) )
		return videoTexture;

	if( !frame.getWidth() || !frame.getHeight() )
		return videoTexture;

	// create or resize texture if needed
	if( !videoTexture )
	{
		videoTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, frame.getWidth(), frame.getHeight() );
	}
	else
	{
		int textureWidth = 0, textureHeight = 0;
		SDL_QueryTexture( videoTexture, nullptr, nullptr, &textureWidth, &textureHeight );
		if( textureWidth != (int)frame.getWidth() || textureHeight != (int)frame.getHeight() )
		{
			SDL_DestroyTexture( videoTexture );
			videoTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, frame.getWidth(), frame.getHeight() );
			std::cerr << std::string(__PRETTY_FUNCTION__) << ": resized to "<< frame.getWidth() << "x" << frame.getHeight() << "\n";
		}
	}

	// SDL only supports 3/4 component images !? need to convert here
	size_t frameSize = frame.size();
	std::vector< uint8_t > frame3( frameSize * 3 );
	for( unsigned int i = 0; i < frameSize; i++ )
	{
		frame3[i*3] = frame[i];
		frame3[i*3+1] = frame[i];
		frame3[i*3+2] = frame[i];
	}

	SDL_UpdateTexture( videoTexture, nullptr, frame3.data(), frame.getWidth() * 3 );

	return videoTexture;
}


static PointIR::DBusClient dbus;

static SDL_Texture * loadCalibrationImage( SDL_Renderer * renderer, unsigned int width, unsigned int height )
{
	std::string filename = dbus.getCalibrationImageFile( width, height );

	std::vector< uint8_t > image;
	unsigned int loadedWidth = 0;
	unsigned int loadedHeight = 0;
	unsigned int error = lodepng::decode( image, loadedWidth, loadedHeight, filename, LodePNGColorType::LCT_GREY, 8 );
	if( error )
		throw RUNTIME_ERROR( "LodePNG encode error " + std::to_string(error) + ": " + lodepng_error_text(error) );
	if( loadedWidth != width || loadedHeight != height )
		throw RUNTIME_ERROR( "PointIR daemon generated a calibration image of different size" );

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


int main( int argc, char ** argv )
{
	////////////////////////////////////////////////////////////////
	// SDL2 init

	//SDL_LogSetAllPriority( SDL_LOG_PRIORITY_DEBUG );
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
	SDL_Texture * calibrationImageTexture = loadCalibrationImage( renderer, w, h );

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
			if( dbus.calibrate() )
			{
				std::cout << "Calibration succeeded :)\n";
				dbus.saveCalibrationData();
			}
			else
			{
				std::cout << "Calibration failed :(\n";
			}
		}

		videoTexture = receiveFrame( renderer );
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

	return 0;
}
