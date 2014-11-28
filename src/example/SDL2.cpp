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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>


int main( int argc, char ** argv )
{
	struct Touch
	{
		SDL_Point point;
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	std::map< int64_t, Touch > touches;

	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS );

	SDL_Window * window = SDL_CreateWindow(
		"PointIR Example SDL2",       // title
		SDL_WINDOWPOS_UNDEFINED,      // position x
		SDL_WINDOWPOS_UNDEFINED,      // position y
		640,                          // width
		480,                          // height
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

	bool close = false;
	while( !close )
	{
		int w = 0, h = 0;
		SDL_GetWindowSize( window, &w, &h );

		SDL_Event e;
		while( SDL_PollEvent( &e ) )
		{
			switch( e.type )
			{
			case SDL_QUIT:
				close = true;
				break;
			case SDL_KEYDOWN:
				switch( e.key.keysym.sym )
				{
				case SDLK_ESCAPE:
					close = true;
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
				}
				break;
			case SDL_FINGERDOWN:
				{
					Touch t;
#ifdef __unix__
					t.point.x = e.tfinger.x;
					t.point.y = e.tfinger.y;
#else
					t.point.x = e.tfinger.x * w;
					t.point.y = e.tfinger.y * h;
#endif
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
#ifdef __unix__
						t.point.x = e.tfinger.x;
						t.point.y = e.tfinger.y;
#else
						t.point.x = e.tfinger.x * w;
						t.point.y = e.tfinger.y * h;
#endif
					}
				}
				break;
			}
		}

		SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
		SDL_RenderClear( renderer );

		for( auto & p : touches )
		{
			const Touch & touch = p.second;
			SDL_SetRenderDrawColor( renderer, touch.r, touch.g, touch.b, 0xff );
			SDL_RenderDrawLine( renderer, 0, touch.point.y, w, touch.point.y );
			SDL_RenderDrawLine( renderer, touch.point.x, 0, touch.point.x, h );
		}

		SDL_RenderPresent( renderer );
	}

	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();
	return 0;
}
