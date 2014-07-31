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

#ifndef _CAPTUREV4L2__INCLUDED_
#define _CAPTUREV4L2__INCLUDED_


#include "ACapture.hpp"

#include <memory>
#include <string>


class CaptureV4L2 : public ACapture
{
public:
	CaptureV4L2( const CaptureV4L2 & ) = delete; // disable copy constructor

	CaptureV4L2( const std::string & device, unsigned int width = 320, unsigned int height = 240, float fps = 30 );
	~CaptureV4L2();

	virtual void start() override;
	virtual unsigned int advanceFrame( bool block = true, float timeoutSeconds = -1.0f ) override;
	virtual PointIR_Frame * retrieveFrame( PointIR_Frame * reuse = nullptr ) const override;
	virtual void stop() override;

	virtual bool isCapturing() const override { return this->capturing; };
	virtual std::string getName() const override;
	virtual unsigned int getWidth() const override { return this->width; }
	virtual unsigned int getHeight() const override { return this->height; }

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


#endif
