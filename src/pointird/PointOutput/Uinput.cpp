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

//#define _POINTOUTPUT_UINPUT__LIVEDEBUG_


#include "Uinput.hpp"
#include "../exceptions.hpp"

#include <PointIR/PointArray.h>

#include <iostream>
#include <vector>

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/input.h>
#include <linux/uinput.h>


using namespace PointOutput;


static const int resX = 4096;
static const int resY = 4096;
static const int fuzzX = 0;
static const int fuzzY = 0;
static const int flatX = 0;
static const int flatY = 0;
static const std::string uinputDeviceName("/dev/uinput");


class Uinput::Impl
{
public:
	int fd = 0;
	bool hadPreviousContact = true;
};


static int xioctl( int fd, unsigned long int request, decltype(EV_ABS) arg )
{
	int r;
	do
		r = ioctl( fd, request, arg );
	while( -1 == r && EINTR == errno );
	return r;
}


static int xioctl( int fd, unsigned long int request )
{
	int r;
	do
		r = ioctl( fd, request );
	while( -1 == r && EINTR == errno );
	return r;
}


Uinput::Uinput() :
	pImpl( new Impl )
{
	this->pImpl->fd = open( uinputDeviceName.c_str(), O_WRONLY | O_NONBLOCK );
	if( this->pImpl->fd < 0 )
		throw SYSTEM_ERROR( errno, "open(\""+uinputDeviceName+"\",O_WRONLY|O_NONBLOCK)" );

	// setup device information
	struct uinput_user_dev uidev = {};
	snprintf( uidev.name, UINPUT_MAX_NAME_SIZE, "PointIR uinput output" );

	uidev.id.bustype = BUS_VIRTUAL;
	uidev.id.vendor  = 0x1; //TODO: choose something different?
	uidev.id.product = 0x1; //TODO: choose something different?
	uidev.id.version = 1;

	uidev.absmax[ABS_MT_POSITION_X] = resX;
	uidev.absmax[ABS_MT_POSITION_Y] = resY;
	uidev.absmin[ABS_MT_POSITION_X] = 0;
	uidev.absmin[ABS_MT_POSITION_Y] = 0;
	uidev.absfuzz[ABS_MT_POSITION_X] = fuzzX;
	uidev.absflat[ABS_MT_POSITION_X] = flatX;
	uidev.absfuzz[ABS_MT_POSITION_Y] = fuzzY;
	uidev.absflat[ABS_MT_POSITION_Y] = flatY;

	//HACK: xorg/udev needs this even for multitouch
	uidev.absmax[ABS_X] = resX;
	uidev.absmax[ABS_Y] = resY;
	uidev.absmin[ABS_X] = 0;
	uidev.absmin[ABS_Y] = 0;
	uidev.absfuzz[ABS_X] = fuzzX;
	uidev.absflat[ABS_X] = flatX;
	uidev.absfuzz[ABS_Y] = fuzzY;
	uidev.absflat[ABS_Y] = flatY;

	if( write( this->pImpl->fd, &uidev, sizeof(uidev) ) != sizeof(uidev) )
		throw SYSTEM_ERROR( errno, "write(\""+uinputDeviceName+"\",uidev,"+std::to_string(sizeof(uidev))+")" );

	if( xioctl( this->pImpl->fd, UI_SET_EVBIT, EV_SYN ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_EVBIT,EV_SYN)" );
/*
	// set direct device - recommended for new drivers - https://www.kernel.org/doc/Documentation/input/event-codes.txt
	if( xioctl( this->pImpl->fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_PROPBIT,INPUT_PROP_DIRECT)" );
*/
	// using touch button - required for userspace to recognize touchscreens
	if( xioctl( this->pImpl->fd, UI_SET_EVBIT, EV_KEY ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_EVBIT,EV_KEY)" );
	if( xioctl( this->pImpl->fd, UI_SET_KEYBIT, BTN_TOUCH ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_KEYBIT,BTN_TOUCH)" );

	// using absolute multitouch positions
	if( xioctl( this->pImpl->fd, UI_SET_EVBIT, EV_ABS ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_EVBIT,EV_ABS)" );
	if( xioctl( this->pImpl->fd, UI_SET_ABSBIT, ABS_MT_POSITION_X ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_ABSBIT,ABS_MT_POSITION_X)" );
	if( xioctl( this->pImpl->fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_ABSBIT,ABS_MT_POSITION_Y)" );

	//HACK: xorg/udev needs this to recognize it as touchscreen?
	if( xioctl( this->pImpl->fd, UI_SET_ABSBIT, ABS_X ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_ABSBIT,ABS_X)" );
	if( xioctl( this->pImpl->fd, UI_SET_ABSBIT, ABS_Y ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_SET_ABSBIT,ABS_Y)" );

	if( xioctl( this->pImpl->fd, UI_DEV_CREATE ) == -1 )
		throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_DEV_CREATE)" );
}


Uinput::~Uinput()
{
	if( this->pImpl->fd )
	{
		if( xioctl( this->pImpl->fd, UI_DEV_DESTROY ) == -1 )
			throw SYSTEM_ERROR( errno, "ioctl(\""+uinputDeviceName+"\",UI_DEV_DESTROY)" );
		if( close( this->pImpl->fd ) == -1 )
			throw SYSTEM_ERROR( errno, "close(\""+uinputDeviceName+"\")" );
	}
}


#ifdef _POINTOUTPUT_UINPUT__LIVEDEBUG_
#include <map>
static std::map< int, std::string > eventTypeStrings =
{
	{EV_KEY,"EV_KEY"},
	{EV_SYN,"EV_SYN"},
	{EV_ABS,"EV_ABS"},
};
static std::map< int, std::string > eventCodeStrings =
{
	{BTN_TOUCH,"BTN_TOUCH"},
	{BTN_TOOL_FINGER,"BTN_TOOL_FINGER"},
	{SYN_MT_REPORT,"SYN_MT_REPORT"},
	{ABS_MT_POSITION_X,"ABS_MT_POSITION_X"},
	{ABS_MT_POSITION_Y,"ABS_MT_POSITION_Y"},
	{SYN_REPORT,"SYN_REPORT"},
};
#endif

static void addEvent( std::vector< struct input_event > & events, __u16 type, __u16 code, __s16 value = 0 )
{
#ifdef _POINTOUTPUT_UINPUT__LIVEDEBUG_
	std::cerr << "PointOutput::Uinput:\ttype= " << eventTypeStrings[type] << "\tcode= " << eventCodeStrings[code] << "\tvalue= " << value << "\n";
#endif
	struct input_event ev = {};
	ev.type = type;
	ev.code = code;
	ev.value = value;
	events.push_back( ev );
}


void Uinput::outputPoints( const PointIR::PointArray & pointArray )
{
	// https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt
	std::vector< struct input_event > events;

	if( pointArray.empty() )
	{
		// if there was a contact last pass, send a SYN_MT_REPORT followed by SYN_REPORT to mark the last contact as lifted
		if( this->pImpl->hadPreviousContact )
		{
//			addEvent( events, EV_KEY, BTN_TOUCH, 0 );
			addEvent( events, EV_SYN, SYN_MT_REPORT );
			this->pImpl->hadPreviousContact = false;
		}
	}
	else
	{
//		if( !this->pImpl->hadPreviousContact )
//		{
//			addEvent( events, EV_KEY, BTN_TOUCH, 1 );
//		}
		for( const PointIR_Point & point : pointArray )
		{
			int16_t x = resX * point.x;
			int16_t y = resY * point.y;

			if( x >= resX )
				x = resX - 1;
			else if( x < 0 )
				x = 0;

			if( y >= resY )
				y = resY - 1;
			else if( y < 0 )
				y = 0;

			addEvent( events, EV_ABS, ABS_MT_POSITION_X, x );
			addEvent( events, EV_ABS, ABS_MT_POSITION_Y, y );
			addEvent( events, EV_SYN, SYN_MT_REPORT );

			this->pImpl->hadPreviousContact = true;
		}
	}

	// if no events to send - we're done
	if( events.empty() )
		return;

	addEvent( events, EV_SYN, SYN_REPORT );

	for( const auto & event : events )
	{
		ssize_t ret = write( this->pImpl->fd, &event, sizeof(event) );
		if( ret != sizeof( struct input_event ) )
			throw SYSTEM_ERROR( errno, "write(\""+uinputDeviceName+"\",event,"+std::to_string(sizeof(event))+") = "+std::to_string(ret) );
#ifdef _POINTOUTPUT_UINPUT__LIVEDEBUG_
		std::cerr << ".";
#endif
	}
#ifdef _POINTOUTPUT_UINPUT__LIVEDEBUG_
	std::cerr << "\n";
#endif

//TODO: the code below doesn't work for older kernels - but would probably be more efficient
/*
	size_t packetSize = events.size() * sizeof(struct input_event);
	size_t toWrite = packetSize;
	size_t written = 0;
	do
	{
		ssize_t ret = write( this->pImpl->fd, events.data()+written, toWrite );
		if( ret <= 0 )
			throw SYSTEM_ERROR( errno, "write(\""+uinputDeviceName+"\",events,"+std::to_string(packetSize)+") = "+std::to_string(ret) );
		written += ret;
		toWrite -= ret;
	} while( toWrite > 0 );

	if( written != packetSize )
		std::cerr << "PointOutputUinput: Failed to write " << events.size() << " events, each " << sizeof(struct input_event) << " bytes - " << written << " of " << packetSize << " bytes written\n";
#ifdef _POINTOUTPUT_UINPUT__LIVEDEBUG_
	else
		std::cerr << "PointOutputUinput: Written " << events.size() << " events, each " << sizeof(struct input_event) << " bytes\n";
#endif
*/
}
