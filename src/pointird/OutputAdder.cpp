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

#include "OutputAdder.hpp"

#include "Processor.hpp"

#include "PointOutput/DebugPointOutputCV.hpp"
#include "PointOutput/PointOutputUinput.hpp"
#include "PointOutput/UnixDomainSocketPointOutput.hpp"

#include "FrameOutput/UnixDomainSocketFrameOutput.hpp"


OutputAdder::OutputAdder()
{
	pointOutputMap.insert( { "uinput", [] ()
		{ return new PointOutputUinput; }
	} );
	pointOutputMap.insert( { "socket", [] ()
		{ return new UnixDomainSocketPointOutput; }
	} );
	pointOutputMap.insert( { "debugcv", [this] () -> APointOutput *
		{
			if( this->processor )
				return new DebugPointOutputCV( *(this->processor) );
			else
				return nullptr;
		}
	} );

	frameOutputMap.insert( { "socket", [] ()
		{ return new UnixDomainSocketFrameOutput; }
	} );
}


bool OutputAdder::addPointOutput( const std::string name )
{
	if( !this->processor )
		return false;
	PointOutputMap::const_iterator it = this->pointOutputMap.find( name );
	if( it == this->pointOutputMap.end() )
		return false;
	APointOutput * output = it->second();
	if( !output )
		return false;
	this->processor->addPointOutput( output );
	return true;
}


bool OutputAdder::addFrameOutput( const std::string name )
{
	if( !this->processor )
		return false;
	FrameOutputMap::const_iterator it = this->frameOutputMap.find( name );
	if( it == this->frameOutputMap.end() )
		return false;
	AFrameOutput * output = it->second();
	if( !output )
		return false;
	this->processor->addFrameOutput( output );
	return true;
}


bool OutputAdder::add( const std::string & name )
{
	bool added = false;
	added |= this->addPointOutput( name );
	added |= this->addFrameOutput( name );
	return added;
}


std::vector< std::string > OutputAdder::getAvailableOutputs()
{
	std::vector< std::string > outputs;
	for( PointOutputMap::const_iterator it = this->pointOutputMap.begin(); it != this->pointOutputMap.end(); ++it )
		outputs.push_back( it->first );
	for( FrameOutputMap::const_iterator it = this->frameOutputMap.begin(); it != this->frameOutputMap.end(); ++it )
	{
		if( !this->pointOutputMap.count( it->first ) ) // no duplicates if frame and point outputs are named equal
			outputs.push_back( it->first );
	}
	return outputs;
}
