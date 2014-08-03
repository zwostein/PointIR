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

#include "OutputFactory.hpp"

#include "Processor.hpp"

#include "PointOutput/DebugPointOutputCV.hpp"
#include "PointOutput/PointOutputUinput.hpp"
#include "PointOutput/UnixDomainSocketPointOutput.hpp"
#include "FrameOutput/UnixDomainSocketFrameOutput.hpp"

#include <map>
#include <functional>


class OutputFactory::Impl
{
public:
	typedef std::function< APointOutput*(void) > PointOutputCreator;
	typedef std::function< AFrameOutput*(void) > FrameOutputCreator;
	typedef std::map< std::string, PointOutputCreator > PointOutputMap;
	typedef std::map< std::string, FrameOutputCreator > FrameOutputMap;

	PointOutputMap pointOutputMap;
	FrameOutputMap frameOutputMap;
};


OutputFactory::OutputFactory() : pImpl( new Impl )
{
	this->pImpl->pointOutputMap.insert( { "uinput", [] ()
		{ return new PointOutputUinput; }
	} );
	this->pImpl->pointOutputMap.insert( { "socket", [] ()
		{ return new UnixDomainSocketPointOutput; }
	} );
	this->pImpl->pointOutputMap.insert( { "debugcv", [this] () -> APointOutput *
		{
			if( this->processor )
				return new DebugPointOutputCV( *(this->processor) );
			else
				return nullptr;
		}
	} );

	this->pImpl->frameOutputMap.insert( { "socket", [] ()
		{ return new UnixDomainSocketFrameOutput; }
	} );
}


OutputFactory::~OutputFactory()
{
}


APointOutput * OutputFactory::newPointOutput( const std::string name ) const
{
	if( !this->processor )
		return nullptr;
	Impl::PointOutputMap::const_iterator it = this->pImpl->pointOutputMap.find( name );
	if( it == this->pImpl->pointOutputMap.end() )
		return nullptr;
	APointOutput * output = it->second();
	return output;
}


AFrameOutput * OutputFactory::newFrameOutput( const std::string name ) const
{
	if( !this->processor )
		return nullptr;
	Impl::FrameOutputMap::const_iterator it = this->pImpl->frameOutputMap.find( name );
	if( it == this->pImpl->frameOutputMap.end() )
		return nullptr;
	AFrameOutput * output = it->second();
	return output;
}


std::vector< std::string > OutputFactory::getAvailablePointOutputs() const
{
	std::vector< std::string > outputs;
	for( Impl::PointOutputMap::const_iterator it = this->pImpl->pointOutputMap.begin(); it != this->pImpl->pointOutputMap.end(); ++it )
		outputs.push_back( it->first );
	return outputs;
}


std::vector< std::string > OutputFactory::getAvailableFrameOutputs() const
{
	std::vector< std::string > outputs;
	for( Impl::FrameOutputMap::const_iterator it = this->pImpl->frameOutputMap.begin(); it != this->pImpl->frameOutputMap.end(); ++it )
		outputs.push_back( it->first );
	return outputs;
}


std::vector< std::string > OutputFactory::getAvailableOutputs() const
{
	std::vector< std::string > outputs;
	for( Impl::PointOutputMap::const_iterator it = this->pImpl->pointOutputMap.begin(); it != this->pImpl->pointOutputMap.end(); ++it )
		outputs.push_back( it->first );
	for( Impl::FrameOutputMap::const_iterator it = this->pImpl->frameOutputMap.begin(); it != this->pImpl->frameOutputMap.end(); ++it )
	{
		if( !this->pImpl->pointOutputMap.count( it->first ) ) // no duplicates if frame and point outputs are named equal
			outputs.push_back( it->first );
	}
	return outputs;
}
