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

#ifndef _CAPTUREFACTORY__INCLUDED_
#define _CAPTUREFACTORY__INCLUDED_


#ifndef POINTIR_CAPTURE_DEFAULT_DEVICENAME
	#ifdef __unix__
		#define POINTIR_CAPTURE_DEFAULT_DEVICENAME "/dev/video0"
	#else
		#define POINTIR_CAPTURE_DEFAULT_DEVICENAME ""
	#endif
#endif
#ifndef POINTIR_CAPTURE_DEFAULT_WIDTH
	#define POINTIR_CAPTURE_DEFAULT_WIDTH       320
#endif
#ifndef POINTIR_CAPTURE_DEFAULT_HEIGHT
	#define POINTIR_CAPTURE_DEFAULT_HEIGHT      240
#endif
#ifndef POINTIR_CAPTURE_DEFAULT_FPS
	#define POINTIR_CAPTURE_DEFAULT_FPS         30.0f
#endif


#include <memory>
#include <string>
#include <vector>


class Processor;
class ACapture;


class CaptureFactory
{
public:
	CaptureFactory();
	~CaptureFactory();

	ACapture * newCapture( const std::string name ) const;

	std::vector< std::string > getAvailableCaptureNames() const;

	std::string deviceName = POINTIR_CAPTURE_DEFAULT_DEVICENAME;
	unsigned int width     = POINTIR_CAPTURE_DEFAULT_WIDTH;
	unsigned int height    = POINTIR_CAPTURE_DEFAULT_HEIGHT;
	float fps              = POINTIR_CAPTURE_DEFAULT_FPS;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};


#endif
