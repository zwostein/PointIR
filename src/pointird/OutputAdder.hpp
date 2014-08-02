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

#ifndef _OUTPUTADDER__INCLUDED_
#define _OUTPUTADDER__INCLUDED_


#include <string>
#include <vector>
#include <map>
#include <functional>


class Processor;
class APointOutput;
class AFrameOutput;


class OutputAdder
{
public:
	OutputAdder();

	bool addPointOutput( const std::string name );
	bool addFrameOutput( const std::string name );
	bool add( const std::string & name );

	std::vector< std::string > getAvailableOutputs();

	void setProcessor( Processor * processor ) { this->processor = processor; }

private:
	typedef std::map< std::string, std::function< APointOutput*(void) > > PointOutputMap;
	typedef std::map< std::string, std::function< AFrameOutput*(void) > > FrameOutputMap;
	PointOutputMap pointOutputMap;
	FrameOutputMap frameOutputMap;

	Processor * processor;
};


#endif
