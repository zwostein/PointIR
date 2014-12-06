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


#include "Win8TouchInjection.hpp"
#include "../exceptions.hpp"
#include "../TrackerFactory.hpp"

#include <PointIR/PointArray.h>

#include <iostream>
#include <vector>

#include <windows.h>


#include "Win8TouchInjection/touchinjection_sdk80.h"


using namespace PointOutput;


class Win8TouchInjection::Impl
{
public:
	Impl()
	{
		HMODULE hMod = LoadLibraryW( L"user32.dll" );
		if( !hMod )
			throw RUNTIME_ERROR( "Could not load user32.dll" );

		// initialize function pointers
		this->InitializeTouchInjection = (InitializeTouchInjectionPtr)GetProcAddress( hMod, "InitializeTouchInjection" );
		if( !this->InitializeTouchInjection )
			throw RUNTIME_ERROR( "Touch Injection API unavailable! (InitializeTouchInjection not found in user32.dll)" );
		this->InjectTouchInput = (InjectTouchInputPtr)GetProcAddress( hMod, "InjectTouchInput" );
		if( !this->InjectTouchInput )
			throw RUNTIME_ERROR( "Touch Injection API unavailable! (InjectTouchInput not found in user32.dll)" );
	}

	InitializeTouchInjectionPtr InitializeTouchInjection = nullptr;
	InjectTouchInputPtr InjectTouchInput = nullptr;

	Tracker::ATracker * tracker = nullptr;
	PointIR::PointArray previousPoints;
	std::vector< int > previousIDs;
	std::vector< int > currentIDs;
	std::vector< int > currentToPrevious;
	std::vector< int > previousToCurrent;
};


bool Win8TouchInjection::isAvailable()
{
	try
	{
		// this will try to initialize function pointers
		Impl impl;
		return impl.InitializeTouchInjection;
	}
	catch(...)
	{
		return false;
	}
}


Win8TouchInjection::Win8TouchInjection( const TrackerFactory & trackerFactory ) :
	pImpl( new Impl )
{
	this->pImpl->tracker = trackerFactory.newTracker( MAX_TOUCH_COUNT );

	if( !this->pImpl->InitializeTouchInjection( this->pImpl->tracker->getMaxID(), TOUCH_FEEDBACK_DEFAULT ) )
		throw RUNTIME_ERROR( "InitializeTouchInjection failure. GetLastError=" + std::to_string(GetLastError()) );

	std::cout << "PointOutput::Win8TouchInjection: initialized for " << this->pImpl->tracker->getMaxID() << " touch points\n";
}


Win8TouchInjection::~Win8TouchInjection()
{
	delete this->pImpl->tracker;
}


static void clampToScreen( POINT & p, int screenWidth, int screenHeight )
{
	if( p.x < 0 )
		p.x = 0;
	else if( p.x >= screenWidth )
		p.x = screenWidth-1;

	if( p.y < 0 )
		p.y = 0;
	else if( p.y >= screenHeight )
		p.y = screenHeight-1;
}


void Win8TouchInjection::outputPoints( const PointIR::PointArray & currentPoints )
{
	this->pImpl->tracker->assignIDs( this->pImpl->previousPoints, this->pImpl->previousIDs,
	                                currentPoints, this->pImpl->currentIDs,
	                                this->pImpl->previousToCurrent, this->pImpl->currentToPrevious );

	int screenWidth = GetSystemMetrics( SM_CXSCREEN );
	int screenHeight = GetSystemMetrics( SM_CYSCREEN );

	std::vector< POINTER_TOUCH_INFO > infos;
	infos.reserve( currentPoints.size() );

	// remove points not found in the current frame
	for( unsigned int i = 0; i < this->pImpl->previousToCurrent.size(); i++ )
	{
		if( this->pImpl->previousIDs[i] < 0 )
			continue;

		if( this->pImpl->previousToCurrent[i] >= 0 )
			continue;

		POINTER_TOUCH_INFO info = {0};
		info.touchFlags = TOUCH_FLAG_NONE;
		info.touchMask = TOUCH_MASK_NONE;
		info.pointerInfo.pointerFlags = POINTER_FLAG_UP;
		info.pointerInfo.pointerType = PT_TOUCH;
		info.pointerInfo.pointerId = this->pImpl->previousIDs[i];
		info.pointerInfo.ptPixelLocation.x = this->pImpl->previousPoints[i].x * screenWidth;
		info.pointerInfo.ptPixelLocation.y = this->pImpl->previousPoints[i].y * screenHeight;
		clampToScreen( info.pointerInfo.ptPixelLocation, screenWidth, screenHeight );

		infos.push_back( info );
//		std::cerr << "Win8TouchInjection: Removed " << info.pointerInfo.pointerId << "\n";
	}

	// handle new and updated points - if any
	if( currentPoints.size() )
	{
		for( unsigned int i = 0; i < currentPoints.size(); i++ )
		{
			if( this->pImpl->currentIDs[i] < 0 )
				continue;

			POINTER_TOUCH_INFO info = {0};
			info.touchFlags = TOUCH_FLAG_NONE;
			info.touchMask = TOUCH_MASK_NONE;

			info.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
			if( this->pImpl->currentToPrevious[i] < 0 ) // not in previous frame - has to be new
				info.pointerInfo.pointerFlags |= POINTER_FLAG_DOWN;
			else // normal update - point just moved
				info.pointerInfo.pointerFlags |= POINTER_FLAG_UPDATE;

			info.pointerInfo.pointerType = PT_TOUCH;
			info.pointerInfo.pointerId = this->pImpl->currentIDs[i];
			info.pointerInfo.ptPixelLocation.x = currentPoints[i].x * screenWidth;;
			info.pointerInfo.ptPixelLocation.y = currentPoints[i].y * screenHeight;
			clampToScreen( info.pointerInfo.ptPixelLocation, screenWidth, screenHeight );

			infos.push_back( info );
//			std::cerr << "Win8TouchInjection: " << ((info.pointerInfo.pointerFlags&POINTER_FLAG_UPDATE)?"Updated ":"Added ") << info.pointerInfo.pointerId << "\n";
		}
	}

	if( !infos.empty() )
	{
		if( !this->pImpl->InjectTouchInput( infos.size(), infos.data() ) )
			std::cerr << "Win8TouchInjection: InjectTouchInput failed with error " << GetLastError() << "\n";
	}

	this->pImpl->previousPoints = currentPoints;
	this->pImpl->previousIDs = this->pImpl->currentIDs;
}
