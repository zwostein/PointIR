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

#include "OpenCV.hpp"
#include "../exceptions.hpp"

#include <PointIR/Frame.h>

#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


using namespace Capture;


class OpenCV::Impl
{
public:
	std::string fileName;
	int deviceNr = 0;
	cv::VideoCapture * videoCapture = nullptr;
};


OpenCV::OpenCV( int deviceNr, unsigned int width, unsigned int height, float fps )
	: pImpl( new Impl ), width(width), height(height), fps(fps)
{
	this->pImpl->deviceNr = deviceNr;
}


OpenCV::OpenCV( const std::string & fileName, unsigned int width, unsigned int height, float fps )
	: pImpl( new Impl ), width(width), height(height), fps(fps)
{
	this->pImpl->fileName = fileName;
}


OpenCV::~OpenCV()
{
	this->stop();
}


void OpenCV::start()
{
	this->stop();

	if( this->pImpl->fileName.empty() )
		this->pImpl->videoCapture = new cv::VideoCapture( this->pImpl->deviceNr );
	else
		this->pImpl->videoCapture = new cv::VideoCapture( this->pImpl->fileName );

	if( !this->pImpl->videoCapture->isOpened() )
	{
		delete this->pImpl->videoCapture;
		this->pImpl->videoCapture = nullptr;
		std::string str;
		if( this->pImpl->fileName.empty() )
			str = std::to_string( this->pImpl->deviceNr );
		else
			str = this->pImpl->fileName;
		throw RUNTIME_ERROR( "Capture::OpenCV: Could not open \"" + str + "\"" );
	}

	this->pImpl->videoCapture->set( CV_CAP_PROP_FRAME_WIDTH, this->width );
	this->pImpl->videoCapture->set( CV_CAP_PROP_FRAME_HEIGHT, this->height );
	this->pImpl->videoCapture->set( CV_CAP_PROP_FPS, this->fps );

	this->capturing = true;
}


void OpenCV::stop()
{
	if( this->pImpl->videoCapture )
	{
		delete this->pImpl->videoCapture;
		this->pImpl->videoCapture = nullptr;
	}

	this->capturing = false;
}


bool OpenCV::advanceFrame( bool block, float timeoutSeconds )
{
	if( !this->pImpl->videoCapture )
		return false;

	bool ret = this->pImpl->videoCapture->grab();

	if( this->pImpl->fileName.empty() )
		return ret;
	else
	{
		if( !ret ) // exit if we played a file to the end
			exit(0);
		else
			return true;
	}
}


bool OpenCV::retrieveFrame( PointIR::Frame & frame ) const
{
	if( !this->pImpl->videoCapture )
		return false;

	cv::Mat colorMat;
	if( !this->pImpl->videoCapture->retrieve( colorMat ) )
		return false;

	cv::Mat greyMat;
	cv::cvtColor( colorMat, greyMat, CV_BGR2GRAY );

	assert( greyMat.isContinuous() );
	frame.resize( greyMat.size().width, greyMat.size().height );
	memcpy( frame.data(), greyMat.data, frame.size() );
	return true;
}
