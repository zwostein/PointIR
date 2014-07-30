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

#include "UnixDomainSocketFrameOutput.hpp"

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
#include <malloc.h>


#define SYSTEM_ERROR( errornumber, whattext ) \
	std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


//HACK: flexible array members (char array[]) are part of C99 but not C++11 and below - however they work with g++, so just disable the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t data[];
} PointIR_Frame;
#pragma GCC diagnostic pop


class UnixDomainSocketFrameOutput::Impl
{
public:
	struct Socket
	{
		struct sockaddr_un addr;
		int fd = 0;
	};
	Socket local;
	std::list< Socket > remotes;
	PointIR_Frame * buffer;
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


UnixDomainSocketFrameOutput::UnixDomainSocketFrameOutput() :
	pImpl( new Impl )
{
	this->pImpl->buffer = (PointIR_Frame*) calloc( 1, sizeof(PointIR_Frame) );
	if( !this->pImpl->buffer)
		throw SYSTEM_ERROR( errno, "calloc" );

	std::stringstream ss;
//	ss << "/tmp/PointIR." << getpid() << ".video.socket";
	ss << "/tmp/PointIR.video.socket";
	this->socketPath = ss.str();

	// delete existing socket if it exists
	unlinkSocket( this->socketPath );

	this->pImpl->local.fd = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
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


UnixDomainSocketFrameOutput::~UnixDomainSocketFrameOutput()
{
	try
	{
		free( this->pImpl->buffer );
		unlinkSocket( this->socketPath );
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


void UnixDomainSocketFrameOutput::outputFrame( const uint8_t * image, unsigned int width, unsigned int height )
{
	size_t size = width * height;
	if( !size )
		return;

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

		// resize socket send buffer to fit one frame - doesn't seem necessary for SOCK_SEQPACKET
		int sndbuf = sizeof(PointIR_Frame) + size;
		setsockopt( newRemote.fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int) );

		this->pImpl->remotes.push_back( newRemote );
	}

	// resize packet buffer if needed
	if( (this->pImpl->buffer->width != width) || (this->pImpl->buffer->height != height) )
	{
		std::cerr << std::string(__PRETTY_FUNCTION__) << ": resizing buffer to "<< width << "x" << height << "\n";

		// resize packet buffer
		this->pImpl->buffer = (PointIR_Frame*) realloc( this->pImpl->buffer, sizeof(PointIR_Frame) + size );
		if( !this->pImpl->buffer )
			throw SYSTEM_ERROR( errno, "realloc" );
		this->pImpl->buffer->width = width;
		this->pImpl->buffer->height = height;

		// resize socket buffers - doesn't seem necessary for SOCK_SEQPACKET
		int sndbuf = sizeof(PointIR_Frame) + size;
		for( auto & remote : this->pImpl->remotes )
			setsockopt( remote.fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int) );

	}

	// copy frame to packet buffer
	memcpy( this->pImpl->buffer->data, image, size );

	// send frame packet - removing remotes on the fly if disconnected
	for( auto it = this->pImpl->remotes.begin(); it != this->pImpl->remotes.end(); )
	{
		ssize_t sent = send( it->fd, this->pImpl->buffer, sizeof(PointIR_Frame) + size, MSG_NOSIGNAL );
		if( -1 == sent )
		{
			if( EPIPE == errno || ECONNRESET == errno )
			{ // remote closed connection
				it = this->pImpl->remotes.erase( it );
				continue;
			}
			else if( EAGAIN == errno || EWOULDBLOCK == errno )
			{ // not enough space left in send buffer
				std::cerr << std::string(__PRETTY_FUNCTION__) << ": remote for descriptor " << it->fd << " too slow - skipping" << "\n";
			}
			else
				throw SYSTEM_ERROR( errno, "send" );
		}
		else if( (size_t)sent != sizeof(PointIR_Frame) + size )
		{ // incomplete transfer - not handled - disconnect to be safe
			std::cerr << std::string(__PRETTY_FUNCTION__) << ": incomplete transfer to remote for descriptor " << it->fd << " - sent " << sent << " of " << (sizeof(PointIR_Frame) + size) << "bytes\n";
			close( it->fd );
			it = this->pImpl->remotes.erase( it );
			continue;
		}
		++it;
	}
}
