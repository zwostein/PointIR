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

#ifndef _AFRAMEOUTPUT__INCLUDED_
#define _AFRAMEOUTPUT__INCLUDED_


#include <stdint.h>


namespace PointIR
{
	class Frame;
}


namespace FrameOutput
{

class AFrameOutput
{
public:
	virtual ~AFrameOutput() {}
	virtual void outputFrame( const PointIR::Frame & frame ) = 0;
};

}


#endif
