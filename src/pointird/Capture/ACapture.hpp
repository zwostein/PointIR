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

#ifndef _ACAPTURE__INCLUDED_
#define _ACAPTURE__INCLUDED_


#include <string>

#include <stdint.h>


namespace PointIR
{
	class Frame;
}


namespace Capture
{

class ACapture
{
public:
	virtual ~ACapture() {}

	virtual void start() = 0;
	virtual bool advanceFrame( bool block = true, float timeoutSeconds = -1.0f ) = 0;
	virtual bool retrieveFrame( PointIR::Frame & frame ) const = 0;
	virtual void stop() = 0;

	virtual bool isCapturing() const = 0;
};

}


#endif
