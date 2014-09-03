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

#ifndef _POINTOUTPUT_DEBUGOPENCV__INCLUDED_
#define _POINTOUTPUT_DEBUGOPENCV__INCLUDED_


#include "APointOutput.hpp"


class Processor;


namespace PointOutput
{

class DebugOpenCV : public APointOutput
{
public:
	DebugOpenCV( const DebugOpenCV & ) = delete; // disable copy constructor
	DebugOpenCV & operator=( const DebugOpenCV & other ) = delete; // disable assignment operator

	DebugOpenCV( const Processor & processor );
	virtual ~DebugOpenCV();

	virtual void outputPoints( const PointIR::PointArray & pointArray ) override;

private:
	const Processor & processor;
};

}


#endif
