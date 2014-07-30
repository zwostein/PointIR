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

#include "UnixDomainSocketPointOutput.hpp"

#include <list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SYSTEM_ERROR( errornumber, whattext ) \
std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


class UnixDomainSocketPointOutput::Impl
{
public:
	struct Socket
	{
		struct sockaddr_un addr;
		int fd = 0;
	};
	Socket local;
	std::list< Socket > remotes;
};


static void unlinkSocket( const std::string & socketPath )
{
	struct stat st;
	if( -1 == stat( socketPath.c_str(), &st ) )
	{
		if( ENOENT == errno )
			return; // not existing
		throw SYSTEM_ERROR( errno, "stat(\"" + socketPath + "\")" );
	}

	if( !S_ISSOCK( st.st_mode ) )
		throw RUNTIME_ERROR( "\"" + socketPath + "\" is not a socket - you have to delete this file manually" );

	if( -1 == unlink( socketPath.c_str() ) )
		throw SYSTEM_ERROR( errno, "unlink(\"" + socketPath + "\")" );
}


UnixDomainSocketPointOutput::UnixDomainSocketPointOutput() :
	pImpl( new Impl )
{
	std::stringstream ss;
	ss << "/tmp/PointIR." << getpid() << ".points.socket";
	this->socketPath = ss.str();

	// delete existing socket if it exists
	unlinkSocket( this->socketPath );

	this->pImpl->local.fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( -1 == this->pImpl->local.fd )
		throw SYSTEM_ERROR( errno, "socket" );

	// set local socket nonblocking
	int flags = fcntl( this->pImpl->local.fd, F_GETFL, 0 );
	if( -1 == flags )
		throw SYSTEM_ERROR( errno, "fcntl" );
	if( -1 == fcntl( this->pImpl->local.fd, F_SETFL, flags | O_NONBLOCK ) )
		throw SYSTEM_ERROR( errno, "fcntl" );

	this->pImpl->local.addr.sun_family = AF_UNIX;
	strcpy( this->pImpl->local.addr.sun_path, this->socketPath.c_str() );

	size_t len = strlen( this->pImpl->local.addr.sun_path ) + sizeof( this->pImpl->local.addr.sun_family );

	mode_t lastUmask = umask( 0 ); // set file creation permission mask for bind to create a socket everyone can connect to
	if( -1 == bind( this->pImpl->local.fd, (struct sockaddr *)&(this->pImpl->local.addr), len ) )
		throw SYSTEM_ERROR( errno, "bind" );
	umask( lastUmask ); // reset to previous file creation permission mask

	if( -1 == listen( this->pImpl->local.fd, 8 ) )
		throw SYSTEM_ERROR( errno, "listen" );
}


UnixDomainSocketPointOutput::~UnixDomainSocketPointOutput()
{
	try
	{
		unlinkSocket( socketPath );
	}
	catch( std::exception & ex )
	{
		std::cerr << std::string(__PRETTY_FUNCTION__) << std::string(": ignoring exception: ") << ex.what() << "\n";
	}
	catch( ... )
	{
		std::cerr << std::string(__PRETTY_FUNCTION__) << std::string(": ignoring unknown exception\n");
	}
}


void UnixDomainSocketPointOutput::outputPoints( const std::vector< PointIR_Point > & points )
{
	// accept all incoming connections
	while( true )
	{
		Impl::Socket newRemote;
		socklen_t len = sizeof( newRemote.addr );
		newRemote.fd = accept( this->pImpl->local.fd, (struct sockaddr *)&(newRemote.addr), &len );
		if( -1 == newRemote.fd )
		{
			if( (EAGAIN==errno) || (EWOULDBLOCK==errno) )
				break; // no incoming connections left
			else
				throw SYSTEM_ERROR( errno, "accept" );
		}

		// set remote socket nonblocking
		int flags = fcntl( newRemote.fd, F_GETFL, 0 );
		if( -1 == flags )
			throw SYSTEM_ERROR( errno, "fcntl" );
		if( -1 == fcntl( newRemote.fd, F_SETFL, flags | O_NONBLOCK ) )
			throw SYSTEM_ERROR( errno, "fcntl" );

		this->pImpl->remotes.push_back( newRemote );
	}

	// send current frame - removing remotes on the fly if disconnected
	for( auto it = this->pImpl->remotes.begin(); it != this->pImpl->remotes.end(); )
	{
		// send number of points following
		uint32_t numPoints = points.size();
		if( -1 == send( it->fd, &numPoints, sizeof(numPoints), MSG_NOSIGNAL ) )
		{
			if( EPIPE == errno )
			{
				it = this->pImpl->remotes.erase( it );
				continue;
			}
			else
				throw SYSTEM_ERROR( errno, "send" );
		}

		// no need to call send when no points are available
		if( !points.size() )
		{
			++it;
			continue; // still need to send the number of current points for each remote
		}

		// send the actual array of points
		if( -1 == send( it->fd, points.data(), points.size() * sizeof(PointIR_Point), MSG_NOSIGNAL ) )
		{
			if( EPIPE == errno )
			{
				it = this->pImpl->remotes.erase( it );
				continue;
			}
			else
				throw SYSTEM_ERROR( errno, "send" );
		}

		++it;
	}
}
