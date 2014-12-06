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


#include "TUIO.hpp"
#include "../exceptions.hpp"
#include "../TrackerFactory.hpp"

#include <PointIR/PointArray.h>

#include <iostream>
#include <vector>

//HACK: liblo headers seem broken: they use WIN32 instead of _WIN32 !?
#ifdef _WIN32
	#define WIN32
#endif
#include <lo/lo.h>


using namespace PointOutput;


class TUIO::Impl
{
public:
	lo_address tuioAddr = nullptr;
	uint32_t frameID = 0;
	lo_timetag lastTimetag;

	Tracker::ATracker * tracker = nullptr;
	PointIR::PointArray previousPoints;
	std::vector< int > previousIDs;
	std::vector< int > currentIDs;
	std::vector< int > currentToPrevious;
	std::vector< int > previousToCurrent;
};


TUIO::TUIO( const TrackerFactory & trackerFactory, std::string address ) :
	pImpl( new Impl )
{
	this->pImpl->tuioAddr = lo_address_new_from_url( address.c_str() );
	if( !this->pImpl->tuioAddr )
		throw RUNTIME_ERROR("Could not start OSC/TUIO server");

	this->pImpl->tracker = trackerFactory.newTracker();

	char * addr = lo_address_get_url(this->pImpl->tuioAddr);
	std::cout << "PointOutput::TUIO: Started server on \"" << addr << "\"\n";
	free( addr );
}


TUIO::~TUIO()
{
	delete this->pImpl->tracker;

	lo_address_free( this->pImpl->tuioAddr );
}


void TUIO::outputPoints( const PointIR::PointArray & currentPoints )
{
	this->pImpl->tracker->assignIDs( this->pImpl->previousPoints, this->pImpl->previousIDs,
	                                 currentPoints, this->pImpl->currentIDs,
	                                 this->pImpl->previousToCurrent, this->pImpl->currentToPrevious );

	lo_message msg;

	lo_timetag timetag;
	lo_timetag_now( &timetag );
	lo_bundle bundle = lo_bundle_new( timetag );

	msg = lo_message_new();
	lo_message_add_string( msg, "source" );
	lo_message_add_string( msg, "PointIR" );
	lo_bundle_add_message( bundle, "/tuio/2Dcur", msg );

	msg = lo_message_new();
	lo_message_add_string( msg, "alive" );
	for( unsigned int i = 0; i < this->pImpl->currentIDs.size(); i++ )
	{
		if( this->pImpl->currentIDs[i] < 0 )
			continue;
		lo_message_add_int32( msg, this->pImpl->currentIDs[i] );
	}
	lo_bundle_add_message( bundle, "/tuio/2Dcur", msg ) ;

	double dt = lo_timetag_diff( timetag, this->pImpl->lastTimetag );
	this->pImpl->lastTimetag = timetag;

	for( unsigned int i = 0; i < currentPoints.size(); i++ )
	{
		if( this->pImpl->currentIDs[i] < 0 )
			continue;

		PointIR::Point diff( 0.0f, 0.0f );
		if( this->pImpl->currentToPrevious[i] >= 0 )
			diff = ( this->pImpl->previousPoints[this->pImpl->currentToPrevious[i]] - currentPoints[i] ) / dt;

		msg = lo_message_new();
		lo_message_add_string( msg, "set" );
		lo_message_add_int32( msg, this->pImpl->currentIDs[i] );
		lo_message_add_float( msg, currentPoints[i].x );
		lo_message_add_float( msg, currentPoints[i].y );
		lo_message_add_float( msg, diff.x );
		lo_message_add_float( msg, diff.y );
		lo_message_add_float( msg, 0.0 );
		lo_bundle_add_message( bundle, "/tuio/2Dcur", msg );
	}

	msg = lo_message_new();
	lo_message_add_string( msg, "fseq" );
	lo_message_add_int32( msg, this->pImpl->frameID++ );
	lo_bundle_add_message( bundle, "/tuio/2Dcur", msg ) ;

	if( lo_send_bundle( this->pImpl->tuioAddr, bundle ) == -1 )
		std::cerr << "PointOutput::TUIO: OSC error " << lo_address_errno(this->pImpl->tuioAddr) <<": " << lo_address_errstr(this->pImpl->tuioAddr) << "\n";

	lo_bundle_free_recursive( bundle ) ;

	this->pImpl->previousPoints = currentPoints;
	this->pImpl->previousIDs = this->pImpl->currentIDs;
}
