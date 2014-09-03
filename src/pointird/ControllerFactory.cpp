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

#include "ControllerFactory.hpp"

#include "Controller/AController.hpp"

#ifdef POINTIR_DBUS
	#include "Controller/DBus.hpp"
#endif

#include <string>
#include <map>
#include <functional>


class ControllerFactory::Impl
{
public:
	typedef std::function< AController*(void) > ControllerCreator;
	typedef std::map< std::string, ControllerCreator > ControllerMap;

	ControllerMap controllerMap;
};


ControllerFactory::ControllerFactory() : pImpl( new Impl )
{
#ifdef POINTIR_DBUS
	this->pImpl->controllerMap.insert( { "dbus", [this] ()
		{ return new Controller::DBus( *(this->processor) ); }
	} );
#endif
}


ControllerFactory::~ControllerFactory()
{
}


AController * ControllerFactory::newController( const std::string name ) const
{
	Impl::ControllerMap::const_iterator it = this->pImpl->controllerMap.find( name );
	if( it == this->pImpl->controllerMap.end() )
		return nullptr;
	AController * output = it->second();
	return output;
}


std::vector< std::string > ControllerFactory::getAvailableControllerNames() const
{
	std::vector< std::string > captureNames;
	for( Impl::ControllerMap::const_iterator it = this->pImpl->controllerMap.begin(); it != this->pImpl->controllerMap.end(); ++it )
		captureNames.push_back( it->first );
	return captureNames;
}
