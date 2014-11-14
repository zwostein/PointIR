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

#include "FrameOutput/AFrameOutput.hpp"
#include "PointOutput/APointOutput.hpp"

#include "PointOutput/DebugOpenCV.hpp"

#ifdef POINTIR_UINPUT
	#include "PointOutput/Uinput.hpp"
#endif

#ifdef POINTIR_UNIXDOMAINSOCKET
	#include "PointOutput/UnixDomainSocket.hpp"
	#include "FrameOutput/UnixDomainSocket.hpp"
#endif

#ifdef POINTIR_TUIO
	#include "PointOutput/TUIO.hpp"
#endif

#ifdef POINTIR_WIN8TOUCHINJECTION
	#include "PointOutput/Win8TouchInjection.hpp"
#endif

#include <map>
#include <functional>


class OutputFactory::Impl
{
public:
	typedef std::function< PointOutput::APointOutput*(void) > PointOutputCreator;
	typedef std::function< FrameOutput::AFrameOutput*(void) > FrameOutputCreator;
	typedef std::map< std::string, PointOutputCreator > PointOutputMap;
	typedef std::map< std::string, FrameOutputCreator > FrameOutputMap;

	PointOutputMap pointOutputMap;
	FrameOutputMap frameOutputMap;
};


OutputFactory::OutputFactory() : pImpl( new Impl )
{
#ifdef POINTIR_UINPUT
	this->pImpl->pointOutputMap.insert( { "uinput", [] ()
		{ return new PointOutput::Uinput; }
	} );
	this->pImpl->pointOutputMap.insert( { "uinputB", [this] ()
		{ return new PointOutput::Uinput( &(this->trackerFactory) ); }
	} );
#endif
#ifdef POINTIR_UNIXDOMAINSOCKET
	this->pImpl->pointOutputMap.insert( { "socket", [] ()
		{ return new PointOutput::UnixDomainSocket; }
	} );
#endif
#ifdef POINTIR_TUIO
	this->pImpl->pointOutputMap.insert( { "tuio", [this] ()
		{ return new PointOutput::TUIO( this->trackerFactory ); }
	} );
#endif
#ifdef POINTIR_WIN8TOUCHINJECTION
	this->pImpl->pointOutputMap.insert( { "win8", [this] ()
		{ return new PointOutput::Win8TouchInjection( this->trackerFactory ); }
	} );
#endif
	this->pImpl->pointOutputMap.insert( { "debugcv", [this] () -> PointOutput::APointOutput *
		{
			if( this->processor )
				return new PointOutput::DebugOpenCV( *(this->processor) );
			else
				return nullptr;
		}
	} );

#ifdef POINTIR_UNIXDOMAINSOCKET
	this->pImpl->frameOutputMap.insert( { "socket", [] ()
		{ return new FrameOutput::UnixDomainSocket; }
	} );
#endif
}


OutputFactory::~OutputFactory()
{
}


PointOutput::APointOutput * OutputFactory::newPointOutput( const std::string name ) const
{
	Impl::PointOutputMap::const_iterator it = this->pImpl->pointOutputMap.find( name );
	if( it == this->pImpl->pointOutputMap.end() )
		return nullptr;
	PointOutput::APointOutput * output = it->second();
	return output;
}


FrameOutput::AFrameOutput * OutputFactory::newFrameOutput( const std::string name ) const
{
	Impl::FrameOutputMap::const_iterator it = this->pImpl->frameOutputMap.find( name );
	if( it == this->pImpl->frameOutputMap.end() )
		return nullptr;
	FrameOutput::AFrameOutput * output = it->second();
	return output;
}


std::vector< std::string > OutputFactory::getAvailablePointOutputNames() const
{
	std::vector< std::string > outputs;
	for( Impl::PointOutputMap::const_iterator it = this->pImpl->pointOutputMap.begin(); it != this->pImpl->pointOutputMap.end(); ++it )
		outputs.push_back( it->first );
	return outputs;
}


std::vector< std::string > OutputFactory::getAvailableFrameOutputNames() const
{
	std::vector< std::string > outputs;
	for( Impl::FrameOutputMap::const_iterator it = this->pImpl->frameOutputMap.begin(); it != this->pImpl->frameOutputMap.end(); ++it )
		outputs.push_back( it->first );
	return outputs;
}


std::vector< std::string > OutputFactory::getAvailableOutputNames() const
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
