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

#include "VideoSocketClient.hpp"


#ifndef __unix__
//TODO: implement for non unix platforms
using namespace PointIR;
class VideoSocketClient::Impl{};
VideoSocketClient::VideoSocketClient( const std::string & path ) : pImpl( new Impl ) {}
VideoSocketClient::~VideoSocketClient() {}
bool VideoSocketClient::receiveFrame( Frame & frame ) const { return false; }
#else


#include <iostream>
#include <stdexcept>
#include <system_error>

#include <unistd.h>
#include <fcntl.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SYSTEM_ERROR( errornumber, whattext ) \
std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


using namespace PointIR;


class VideoSocketClient::Impl
{
private:
	mutable int socketFD = 0;

public:
	std::string socketName;

	Impl() {}

	~Impl()
	{
		if( this->socketFD )
			close( this->socketFD );
	}

	bool connectIfNeeded() const
	{
		if( this->socketFD )
			return true;

		this->socketFD = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
		if( -1 == this->socketFD )
			throw SYSTEM_ERROR( errno, "socket" );

		// set local socket nonblocking
		int flags = fcntl( this->socketFD, F_GETFL, 0 );
		if( -1 == flags )
			throw SYSTEM_ERROR( errno, "fcntl" );
		if( -1 == fcntl( this->socketFD, F_SETFL, flags | O_NONBLOCK ) )
			throw SYSTEM_ERROR( errno, "fcntl" );

		struct sockaddr_un remoteAddr;
		remoteAddr.sun_family = AF_UNIX;
		strcpy( remoteAddr.sun_path, this->socketName.c_str() );
		size_t len = strlen(remoteAddr.sun_path) + sizeof(remoteAddr.sun_family);
		if( -1 == connect( this->socketFD, (struct sockaddr *)&remoteAddr, len ) )
		{
//			throw SYSTEM_ERROR( errno, "connect" );
			close( this->socketFD );
			this->socketFD = 0;
			return false;
		}

		return true;
	}

	bool receiveFrame( Frame & frame ) const
	{
		if( !this->connectIfNeeded() )
			return false;

		ssize_t received;

		// peek at the next packet - return if nothing received
		PointIR_Frame peek;
		received = recv( this->socketFD, &peek, sizeof(peek), MSG_PEEK );
		if( -1 == received )
		{
			if( EAGAIN == errno || EWOULDBLOCK == errno )
				return false;
			else
				throw SYSTEM_ERROR( errno, "recv" );
		}
		if( sizeof(peek) != received )
		{
//			throw RUNTIME_ERROR( "too few data received" );
			close( this->socketFD );
			this->socketFD = 0;
			return false;
		}

		// resize packet buffer if needed
		frame.resize( peek.width, peek.height );

		// receive the packet
		received = recv( this->socketFD, (PointIR_Frame*)frame, sizeof(PointIR_Frame)+frame.size(), 0 );
		if( -1 == received )
			throw SYSTEM_ERROR( errno, "recv" );
		if( sizeof(PointIR_Frame)+frame.size() != (size_t)received )
		{
//			throw RUNTIME_ERROR( "too few data received" );
			close( this->socketFD );
			this->socketFD = 0;
			return false;
		}
		return true;
	}
};


VideoSocketClient::VideoSocketClient( const std::string & path ) : pImpl( new Impl )
{
	this->pImpl->socketName = path;
}


VideoSocketClient::~VideoSocketClient()
{
}


bool VideoSocketClient::receiveFrame( Frame & frame ) const
{
	return this->pImpl->receiveFrame( frame );
}


#endif
