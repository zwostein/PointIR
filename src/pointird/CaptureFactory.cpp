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

#include "CaptureFactory.hpp"

#include "Capture/ACapture.hpp"

#include "Capture/CaptureCV.hpp"

#ifdef POINTIR_V4L2
	#include "Capture/CaptureV4L2.hpp"
#endif

#include <string>
#include <map>
#include <functional>


class CaptureFactory::Impl
{
public:
	typedef std::function< ACapture*(void) > CaptureCreator;
	typedef std::map< std::string, CaptureCreator > CaptureMap;

	CaptureMap captureMap;
};


CaptureFactory::CaptureFactory() : pImpl( new Impl )
{
#ifdef POINTIR_V4L2
	this->pImpl->captureMap.insert( { "v4l2", [this] ()
		{ return new CaptureV4L2( this->deviceName, this->width, this->height, this->fps ); }
	} );
#endif
	this->pImpl->captureMap.insert( { "cv", [this] ()
		{
			int devNum = -1;
			try {
				devNum = std::stoul(this->deviceName);
			} catch( ... ) {
				devNum = -1;
			}
			if( devNum < 0 )
				return new CaptureCV( this->deviceName, this->width, this->height, this->fps );
			else
				return new CaptureCV( devNum, this->width, this->height, this->fps );
		}
	} );
}


CaptureFactory::~CaptureFactory()
{
}


ACapture * CaptureFactory::newCapture( const std::string name ) const
{
	Impl::CaptureMap::const_iterator it = this->pImpl->captureMap.find( name );
	if( it == this->pImpl->captureMap.end() )
		return nullptr;
	ACapture * output = it->second();
	return output;
}


std::vector< std::string > CaptureFactory::getAvailableCaptureNames() const
{
	std::vector< std::string > captureNames;
	for( Impl::CaptureMap::const_iterator it = this->pImpl->captureMap.begin(); it != this->pImpl->captureMap.end(); ++it )
		captureNames.push_back( it->first );
	return captureNames;
}
