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

#include "Processor.hpp"

#include "Capture/ACapture.hpp"
#include "FrameOutput/AFrameOutput.hpp"
#include "PointDetector/APointDetector.hpp"
#include "Unprojector/AUnprojector.hpp"
#include "Unprojector/AAutoUnprojector.hpp"
#include "PointFilter/APointFilter.hpp"
#include "PointOutput/APointOutput.hpp"

#include <PointIR/Point.h>

#include <malloc.h>

#include <iostream>
#include <set>


class Processor::Impl
{
public:
	APointFilter * filter = nullptr;

	std::set< AFrameOutput * > frameOutputs;
	std::set< APointOutput * > pointOutputs;
	bool frameOutputEnabled = true;
	bool pointOutputEnabled = true;

	std::function< void(bool) > calibrationResultCallback;
	std::function< void(void) > calibrationBeginCallback;
	std::function< void(void) > calibrationEndCallback;
	bool calibrating = false;
	bool calibrationSucceeded = false;

	void endCalibration( bool result )
	{
		this->calibrationSucceeded = result;
		if( this->calibrationResultCallback )
		{
			this->calibrationResultCallback( result );
			this->calibrationResultCallback = nullptr;
		}
		this->calibrating = false;
		if( this->calibrationEndCallback )
			this->calibrationEndCallback();
	}
};


Processor::Processor( ACapture & capture, APointDetector & detector, AUnprojector & unprojector ) :
	pImpl( new Impl() ),
	capture(capture),
	detector(detector),
	unprojector(unprojector)
{
}


Processor::~Processor()
{
	free( this->frame );
}


void Processor::start()
{
	if( this->isProcessing() )
		return;

	this->capture.start();
}


void Processor::stop()
{
	if( !this->isProcessing() )
		return;
	this->capture.stop();
}


bool Processor::isProcessing() const
{
	return this->capture.isCapturing();
}


void Processor::processFrame()
{
	if( !this->isProcessing() )
		return;

	this->capture.advanceFrame();
	this->frame = this->capture.retrieveFrame( this->frame );

	if( this->pImpl->frameOutputEnabled )
	{
		for( AFrameOutput * output : this->pImpl->frameOutputs )
		output->outputFrame( this->frame );
	}

	if( this->isCalibrating() )
	{
		//TODO: as soon as there are multiple ways for calibrating, move the calibration logic to an external module/class
		if( AAutoUnprojector * autoUnprojector = dynamic_cast<AAutoUnprojector*>( &(this->unprojector) ) )
		{
			bool result = autoUnprojector->calibrate( this->frame->data, this->frame->width, this->frame->height );
			//TODO: maybe keep trying to calibrate for a few frames until calling back
			this->pImpl->endCalibration( result );
		}
		else
		{
			// no calibration supported
			this->pImpl->endCalibration( false );
		}
	}
	else
	{
		std::vector< PointIR_Point > points = detector.detect( this->frame );

		this->unprojector.unproject( points );

		if( this->pImpl->filter )
			this->pImpl->filter->filterPoints( points );

		if( this->pImpl->pointOutputEnabled )
		{
			for( APointOutput * output : this->pImpl->pointOutputs )
				output->outputPoints( points );
		}
	}
}


bool Processor::startCalibration( std::function< void(bool) > resultCallback )
{
	if( this->isCalibrating() )
		return false;
	this->pImpl->calibrationResultCallback = resultCallback;
	this->pImpl->calibrating = true;
	this->pImpl->calibrationSucceeded = false;
	if( this->pImpl->calibrationBeginCallback )
		this->pImpl->calibrationBeginCallback();
	return true;
}


void Processor::setCalibrationBeginCallback( std::function< void(void) > beginCallback )
{
	this->pImpl->calibrationBeginCallback = beginCallback;
}


void Processor::setCalibrationEndCallback( std::function< void(void) > endCallback )
{
	this->pImpl->calibrationEndCallback = endCallback;
}


bool Processor::isCalibrating() const
{
	return this->pImpl->calibrating;
}


bool Processor::isCalibrationSucceeded() const
{
	return this->pImpl->calibrationSucceeded;
}


bool Processor::addFrameOutput( AFrameOutput * output )
{
	return this->pImpl->frameOutputs.insert( output ).second;
}


bool Processor::removeFrameOutput( AFrameOutput * output )
{
	return this->pImpl->frameOutputs.erase( output );
}


bool Processor::addPointOutput( APointOutput * output )
{
	return this->pImpl->pointOutputs.insert( output ).second;
}


bool Processor::removePointOutput( APointOutput * output )
{
	return this->pImpl->pointOutputs.erase( output );
}


void Processor::setFrameOutputEnabled( bool enable )
{
	this->pImpl->frameOutputEnabled = enable;
}


bool Processor::isFrameOutputEnabled() const
{
	return this->pImpl->frameOutputEnabled;
}


void Processor::setPointOutputEnabled( bool enable )
{
	this->pImpl->pointOutputEnabled = enable;
}


bool Processor::isPointOutputEnabled() const
{
	return this->pImpl->pointOutputEnabled;
}


void Processor::setPointFilter( APointFilter * pointFilter )
{
	this->pImpl->filter = pointFilter;
}


APointFilter * Processor::getPointFilter() const
{
	return this->pImpl->filter;
}
