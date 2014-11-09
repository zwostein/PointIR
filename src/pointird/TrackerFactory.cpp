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

#include "TrackerFactory.hpp"
#include "exceptions.hpp"

#include "Tracker/Simple.hpp"
#include "Tracker/Hungarian.hpp"

#include <cassert>
#include <map>
#include <functional>


class TrackerFactory::Impl
{
public:
	typedef std::function< Tracker::ATracker*(void) > TrackerCreator;
	typedef std::map< std::string, TrackerCreator > TrackerMap;

	TrackerMap trackerMap;
	std::string defaultTrackerName;
};


TrackerFactory::TrackerFactory() : pImpl( new Impl )
{
	this->pImpl->trackerMap.insert( { "simple", [] ()
		{ return new Tracker::Simple; }
	} );

	this->pImpl->trackerMap.insert( { "hungarian", [] ()
		{ return new Tracker::Hungarian; }
	} );

	this->setDefaultTrackerName("simple");
}


TrackerFactory::~TrackerFactory()
{
}


std::string TrackerFactory::getDefaultTrackerName() const
{
	return this->pImpl->defaultTrackerName;
}


void TrackerFactory::setDefaultTrackerName( const std::string name )
{
	Impl::TrackerMap::const_iterator it = this->pImpl->trackerMap.find( name );
	if( it == this->pImpl->trackerMap.end() )
		throw RUNTIME_ERROR("Unknown tracker");
	this->pImpl->defaultTrackerName = name;
}


Tracker::ATracker * TrackerFactory::newTracker( const std::string name ) const
{
	Impl::TrackerMap::const_iterator it = this->pImpl->trackerMap.find( name );
	if( it == this->pImpl->trackerMap.end() )
		it = this->pImpl->trackerMap.find( this->pImpl->defaultTrackerName );
	if( it == this->pImpl->trackerMap.end() )
		return nullptr;
	Tracker::ATracker * tracker = it->second();
	return tracker;
}


std::vector< std::string > TrackerFactory::getAvailableTrackerNames() const
{
	std::vector< std::string > trackers;
	for( Impl::TrackerMap::const_iterator it = this->pImpl->trackerMap.begin(); it != this->pImpl->trackerMap.end(); ++it )
		trackers.push_back( it->first );
	return trackers;
}
