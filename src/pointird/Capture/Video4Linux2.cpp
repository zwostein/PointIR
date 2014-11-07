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

#include "Video4Linux2.hpp"
#include "../exceptions.hpp"

#include <PointIR/Frame.h>

#include <iostream>
#include <vector>
#include <limits>

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <malloc.h>


using namespace Capture;


// Based upon: http://rosettacode.org/wiki/Convert_decimal_number_to_rational
template< typename T >
static void rat_approx( double f, T maxDenom, T * num, T * denom )
{
	static_assert( std::numeric_limits<T>::is_integer, "numerator/denominator must be integral type" );

	// a: continued fraction coefficients
	T a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	T x, d, n = 1;
	int i, neg = 0;

	if( maxDenom <= 1 )
	{
		*denom = 1;
		*num = (T) f;
		return;
	}

	if( f < 0 )
	{
		neg = 1;
		f = -f;
	}

	while( f != floor(f) )
	{
		n <<= 1;
		f *= 2;
	}
	d = f;

	// continued fraction and check denominator each step
	for( i = 0; i < 64; i++ )
	{
		a = n ? d / n : 0;
		if( i && !a ) break;

		x = d; d = n; n = x % n;

		x = a;
		if( k[1] * a + k[0] >= maxDenom )
		{
			x = (maxDenom - k[0]) / k[1];
			if( x * 2 >= a || k[1] >= maxDenom )
				i = 65;
			else
				break;
		}

		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
	}

	*denom = k[1];
	*num = neg ? -h[1] : h[1];
}


class Video4Linux2::Impl
{
public:
	typedef struct
	{
		void * start;
		size_t length;
	} Buffer;

	int fd = 0;
	std::vector<Buffer> buffers;
	unsigned int minBufferCount = 2;
	int currentBuffer = -1;
	unsigned int bytesPerLine = 0;
	struct v4l2_capability caps = {};
};


static int xioctl( int fd, unsigned long int request, void * arg )
{
	int r;
	do
		r = ioctl( fd, request, arg );
	while( -1 == r && EINTR == errno );
	return r;
}


static bool getClosestFrameInterval( int fd, float fps, const struct v4l2_pix_format * format, struct v4l2_fract * intervalFract )
{
	struct v4l2_frmivalenum ivalenum = {};
	ivalenum.index = 0;
	ivalenum.pixel_format = format->pixelformat;
	ivalenum.width = format->width;
	ivalenum.height = format->height;
	float selectedFractError = std::numeric_limits< float >::max();
	while( -1 != xioctl( fd, VIDIOC_ENUM_FRAMEINTERVALS, &ivalenum ) )
	{
		switch( ivalenum.type )
		{
		case V4L2_FRMIVAL_TYPE_DISCRETE:
			{
				float ival = static_cast<float>(ivalenum.discrete.numerator) / static_cast<float>(ivalenum.discrete.denominator);
				float error = fabs( ival - 1.0f/fps );
				if( error < selectedFractError )
				{
					*intervalFract = ivalenum.discrete;
					selectedFractError = error;
				}
			}
			break;
		case V4L2_FRMIVAL_TYPE_STEPWISE:
			//TODO:: implement
			throw RUNTIME_ERROR( "V4L2_FRMIVAL_TYPE_STEPWISE is unimplemented" );
			break;
		case V4L2_FRMIVAL_TYPE_CONTINUOUS:
			{
				float ivalMin = static_cast<float>(ivalenum.stepwise.min.numerator) / static_cast<float>(ivalenum.stepwise.min.denominator);
				float ivalMax = static_cast<float>(ivalenum.stepwise.max.numerator) / static_cast<float>(ivalenum.stepwise.max.denominator);
				float thisIval = 1.0f/fps;
				if( thisIval < ivalMin )
				{
					*intervalFract = ivalenum.stepwise.min;
				}
				else if( thisIval > ivalMax )
				{
					*intervalFract = ivalenum.stepwise.max;
				}
				else
				{
					rat_approx<decltype(intervalFract->denominator)>( thisIval, 1000, &(intervalFract->numerator), &(intervalFract->denominator) );
					float ival = static_cast<float>(intervalFract->numerator) / static_cast<float>(intervalFract->denominator);
					selectedFractError = fabs( ival - 1.0f/fps );
				}
			}
			break;
		}
		ivalenum.index++;
	}
	return intervalFract->denominator != 0;
}


Video4Linux2::Video4Linux2( const std::string & device, unsigned int width, unsigned int height, float fps )
	: pImpl( new Impl ), device(device), width(width), height(height), fps(fps)
{
	// check if device exists
	struct stat st;
	if( -1 == stat( this->device.c_str(), &st ) )
		throw SYSTEM_ERROR( errno, "stat(\"" + this->device + "\")" );

	if( !S_ISCHR( st.st_mode ) )
		throw RUNTIME_ERROR( "\"" + this->device + "\" is not a device" );

	// open the device
	this->pImpl->fd = ::open( this->device.c_str(), O_RDWR | O_NONBLOCK );

	if( -1 == this->pImpl->fd )
		throw SYSTEM_ERROR( errno, "open(\"" + this->device + "\",O_RDWR|O_NONBLOCK)" );

	// check if device can capture and stream video
	if( -1 == xioctl( this->pImpl->fd, VIDIOC_QUERYCAP, &this->pImpl->caps ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_QUERYCAP)" );

	if( !this->canCaptureVideo() )
		throw RUNTIME_ERROR( "\"" + this->device + "\" cannot capture video" );

	if( !this->canStream() )
		throw RUNTIME_ERROR( "\"" + this->device + "\" cannot stream" );
/*
	struct v4l2_cropcap cropcap = {};
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( 0 == xioctl( this->pImpl->fd, VIDIOC_CROPCAP, &cropcap ) )
	{
		struct v4l2_crop crop = {};
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; // reset to default
		if( -1 == xioctl( this->pImpl->fd, VIDIOC_S_CROP, &crop ) )
		{
			switch( errno )
			{
			case EINVAL:
				break; // Cropping not supported.
			default:
				break; // Errors ignored.
			}
		}
	}
*/
	// try to set desired format - warn if driver changes format
	struct v4l2_format fmt = {};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = this->width;
	fmt.fmt.pix.height      = this->height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;
	if( -1 == xioctl( this->pImpl->fd, VIDIOC_S_FMT, &fmt ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_S_FMT)" );

	if( fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV )
		throw RUNTIME_ERROR( "\"" + this->device + "\" does not support YUYV pixel format" );

	if( this->width != fmt.fmt.pix.width || this->height != fmt.fmt.pix.height )
	{
		std::cerr << "Capture::Video4Linux2: \"" << this->device << "\": " << "request of size " << this->width << "x" << this->height
			<< " failed, using " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << " instead\n";
	}

	this->width = fmt.fmt.pix.width;
	this->height = fmt.fmt.pix.height;
	this->pImpl->bytesPerLine = fmt.fmt.pix.bytesperline;

	// find and set closest frame interval if possible
	struct v4l2_streamparm streamParm = {};
	streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamParm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	if( getClosestFrameInterval( this->pImpl->fd, this->fps, &(fmt.fmt.pix), &(streamParm.parm.capture.timeperframe) ) )
	{
		if( -1 == xioctl( this->pImpl->fd, VIDIOC_S_PARM, &streamParm ) )
			throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_S_PARM)" );
	}
	else
	{
		std::cerr << "Capture::Video4Linux2: \"" << this->device << "\": " << "Could not find any supported frame interval setting\n";
	}

		std::cout << "Capture::Video4Linux2: \"" << this->device << "\": " << "Selected format " << this->width << "x" << this->height
		<< " @ " << streamParm.parm.capture.timeperframe.numerator << "/" << streamParm.parm.capture.timeperframe.denominator << " s frame interval\n";

	// setup memory mapped stream
	struct v4l2_requestbuffers req = {};
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	req.count  = this->pImpl->minBufferCount;
	if( -1 == xioctl( this->pImpl->fd, VIDIOC_REQBUFS, &req ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_REQBUFS)" );

	if( req.count < this->pImpl->minBufferCount )
		throw RUNTIME_ERROR( "\"" + this->device + "\": Could not acquire required buffers - requested "
			+ std::to_string(this->pImpl->minBufferCount) + " got " + std::to_string(req.count) );

	for( size_t i = 0; i < req.count; ++i )
	{
		struct v4l2_buffer buf = {};
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		if( -1 == xioctl( this->pImpl->fd, VIDIOC_QUERYBUF, &buf ) )
			throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_QUERYBUF)" );

		Impl::Buffer newBuffer;
		newBuffer.length = buf.length;
		newBuffer.start = mmap( NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->pImpl->fd, buf.m.offset );
		if( MAP_FAILED == newBuffer.start )
			throw SYSTEM_ERROR( errno, "mmap( NULL, " + std::to_string(buf.length)
				+ ", PROT_READ | PROT_WRITE, MAP_SHARED, \"" + this->device + "\", " + std::to_string(buf.m.offset) );
		this->pImpl->buffers.push_back( newBuffer );
	}
}


Video4Linux2::~Video4Linux2()
{
	try
	{
		for( Impl::Buffer & buffer : this->pImpl->buffers )
		{
			if( -1 == munmap( buffer.start, buffer.length ) )
				throw SYSTEM_ERROR( errno, "munmap" );
		}

		if( this->pImpl->fd )
		{
			if( -1 == ::close( this->pImpl->fd ) )
				throw SYSTEM_ERROR( errno, "close(\"" + this->device + "\")" );
		}

		this->pImpl->fd = 0;
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


void Video4Linux2::start()
{
	for( size_t i = 0; i < this->pImpl->buffers.size(); ++i )
	{
		struct v4l2_buffer buf = {};
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		if( -1 == xioctl( this->pImpl->fd, VIDIOC_QBUF, &buf ) )
			throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_QBUF)" );
	}
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( -1 == xioctl( this->pImpl->fd, VIDIOC_STREAMON, &type ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_STREAMON)" );

	this->capturing = true;
}


void Video4Linux2::stop()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( -1 == xioctl( this->pImpl->fd, VIDIOC_STREAMOFF, &type ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_STREAMOFF)" );

	this->capturing = false;
}


bool Video4Linux2::advanceFrame( bool block, float timeoutSeconds )
{
	if( block )
	{
		while( true )
		{
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO( &fds );
			FD_SET( this->pImpl->fd, &fds );

			if( timeoutSeconds <= 0.0f )
			{
				r = select( this->pImpl->fd + 1, &fds, NULL, NULL, NULL );
			}
			else
			{
				tv.tv_sec = timeoutSeconds;
				tv.tv_usec = (timeoutSeconds-(int)timeoutSeconds)/1000000.0f;
				r = select( this->pImpl->fd + 1, &fds, NULL, NULL, &tv );
			}

			if( -1 == r )
			{
				if( EINTR == errno )
					continue;
				throw SYSTEM_ERROR( errno, "select" );
			}

			if( 0 == r )
			{
				std::cerr << "\"" << this->device << "\": " << "timed out\n";
				return false;
			}
			break;
		}
	}

	struct v4l2_buffer buf = {0};
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if( -1 == xioctl( this->pImpl->fd, VIDIOC_DQBUF, &buf ) )
	{
		switch( errno )
		{
		case EAGAIN:
			return false;
		case EIO:
			// Could ignore EIO, see spec.
			// fall through
		default:
			throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_DQBUF)" );
		}
	}

	if( buf.index >= this->pImpl->buffers.size() )
		throw RUNTIME_ERROR( "\"" + this->device + "\" returned buffer index out of range - expected maximum "
			+ std::to_string(this->pImpl->buffers.size()) + " but got " + std::to_string(buf.index) );

	if( -1 == xioctl( pImpl->fd, VIDIOC_QBUF, &buf ) )
		throw SYSTEM_ERROR( errno, "ioctl(\"" + this->device + "\",VIDIOC_QBUF)" );

	this->pImpl->currentBuffer = buf.index;
	return true;
}


bool Video4Linux2::retrieveFrame( PointIR::Frame & frame ) const
{
	if( this->pImpl->currentBuffer < 0 )
	{
		std::cerr << "Capture::Video4Linux2: no buffer available\n";
		return false;
	}

	frame.resize( this->width, this->height );

	uint8_t * src = static_cast<uint8_t*>( this->pImpl->buffers[this->pImpl->currentBuffer].start );
	uint8_t * dst = frame.getData();

	// copy greyscale component to destination buffer
	for( unsigned int h = 0; h < this->height; h++ )
	{
		for( unsigned int i = 0; i < this->width-1; i += 2 )
		{
			*dst++ = src[0];
			*dst++ = src[2];
			src += 4;
		}
		src += this->pImpl->bytesPerLine - ( this->width * 2 );
	}
	return true;
}


std::string Video4Linux2::getName() const
{
	return std::string( reinterpret_cast< const char * >(this->pImpl->caps.card) );
}


bool Video4Linux2::canCaptureVideo() const
{
	return this->pImpl->caps.capabilities & V4L2_CAP_VIDEO_CAPTURE;
}


bool Video4Linux2::canStream() const
{
	return this->pImpl->caps.capabilities & V4L2_CAP_STREAMING;
}

/*
bool CaptureV4L2::printCaps()
{
	struct v4l2_capability caps = {};
	if( -1 == xioctl( pimpl->fd, VIDIOC_QUERYCAP, &caps ) )
	{
		perror( "VIDIOC_QUERYCAP" );
		return false;
	}
	std::string capsFlags;
	if( caps.capabilities & V4L2_CAP_VIDEO_CAPTURE )
		capsFlags.append(" VideoCapture ");
	if( caps.capabilities & V4L2_CAP_STREAMING )
		capsFlags.append(" Streaming ");
	if( !capsFlags.empty() )
		capsFlags = "[" + capsFlags + "]";

	std::cout << "\nDevice capabilities:\n"
		<< "\tDriver: \"" << caps.driver << "\"\n"
		<< "\tCard: \"" << caps.card << "\"\n"
		<< "\tBus: \"" << caps.bus_info << "\"\n"
		<< "\tVersion: "
			<< (unsigned int)((caps.version>>16)&0xff)
			<< "."
			<< (unsigned int)((caps.version>>8)&0xff)
			<< "."
			<< (unsigned int)((caps.version)&0xff)
			<< "\n"
		<< "\tCapabilities: " << std::hex << caps.capabilities << " " << capsFlags << "\n";

	struct v4l2_fmtdesc fmtdesc = {0};
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	std::cout << "\nSupported formats:\n";
	while( 0 == xioctl( pimpl->fd, VIDIOC_ENUM_FMT, &fmtdesc ))
	{
		std::string fourcc( (char*)&fmtdesc.pixelformat, 4 );
		std::string flags;
		if( fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED )
			flags.append(" Compressed ");
		if( fmtdesc.flags & V4L2_FMT_FLAG_EMULATED )
			flags.append(" Emulated ");
		if( !flags.empty() )
			flags = "[" + flags + "]";
		std::cout << "\t* " << fourcc << " : \"" << fmtdesc.description << "\" " << flags << "\n";
		fmtdesc.index++;
	}

	std::cout << "\n";
	return true;
}
*/
