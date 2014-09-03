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

#ifndef _CAPTURE_V4L2__INCLUDED_
#define _CAPTURE_V4L2__INCLUDED_


#include "ACapture.hpp"

#include <memory>
#include <string>


namespace Capture
{

class Video4Linux2 : public ACapture
{
public:
	Video4Linux2( const Video4Linux2 & ) = delete; // disable copy constructor
	Video4Linux2 & operator=( const Video4Linux2 & other ) = delete; // disable assignment operator

	Video4Linux2( const std::string & device, unsigned int width = 320, unsigned int height = 240, float fps = 30 );
	virtual ~Video4Linux2();

	virtual void start() override;
	virtual bool advanceFrame( bool block = true, float timeoutSeconds = -1.0f ) override;
	virtual bool retrieveFrame( PointIR::Frame & frame ) const override;
	virtual void stop() override;

	virtual bool isCapturing() const override { return this->capturing; };

	std::string getName() const;
	unsigned int getWidth() const { return this->width; }
	unsigned int getHeight() const { return this->height; }
	std::string getDevice() const { return this->device; }
	bool canStream() const;
	bool canCaptureVideo() const;

	float getFPS() const { return this->fps; }

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
	std::string device;
	unsigned int width;
	unsigned int height;
	float fps;
	bool capturing = false;
};

}


#endif
