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
	Impl( Processor & processor ) : processor(processor) {}

	APointFilter * filter = nullptr;

	std::set< AFrameOutput * > frameOutputs;
	std::set< APointOutput * > pointOutputs;
	bool frameOutputEnabled = true;
	bool pointOutputEnabled = true;

	std::set< ACalibrationListener * > calibrationListeners;
	bool calibrating = false;
	bool calibrationSucceeded = false;

	void endCalibration( bool result )
	{
		this->calibrationSucceeded = result;
		this->calibrating = false;

		for( auto it = this->calibrationListeners.begin(); it != this->calibrationListeners.end(); )
		{
			auto current = it;
			it++;
			(*current)->calibrationEnd( result );
		}

		// flush video buffers
		this->processor.capture.stop();
		this->processor.capture.start();
	}

private:
	Processor & processor;
};


Processor::Processor( ACapture & capture, APointDetector & detector, AUnprojector & unprojector ) :
	pImpl( new Impl( *this ) ),
	capture(capture),
	detector(detector),
	unprojector(unprojector)
{
}


Processor::~Processor()
{
}


std::set< AFrameOutput * > Processor::getFrameOutputs()
{
	return this->pImpl->frameOutputs;
}


std::set< APointOutput * > Processor::getPointOutputs()
{
	return this->pImpl->pointOutputs;
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

	if( !this->capture.advanceFrame() )
	{
		std::cerr << "Processor: Could not get next frame.\n";
		return;
	}
	if( !this->capture.retrieveFrame( this->frame ) )
	{
		std::cerr << "Processor: Could not retrieve frame.\n";
		return;
	}

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
			bool result = autoUnprojector->calibrate( this->frame );
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
		this->detector.detect( this->pointArray, this->frame );

		this->unprojector.unproject( this->pointArray );

		if( this->pImpl->filter )
			this->pImpl->filter->filterPoints( this->pointArray );

		if( this->pImpl->pointOutputEnabled )
		{
			for( APointOutput * output : this->pImpl->pointOutputs )
				output->outputPoints( this->pointArray );
		}
	}
}


bool Processor::startCalibration()
{
	if( this->isCalibrating() )
		return false;

	this->pImpl->calibrating = true;
	this->pImpl->calibrationSucceeded = false;

	for( auto it = this->pImpl->calibrationListeners.begin(); it != this->pImpl->calibrationListeners.end(); )
	{
		auto current = it;
		it++;
		(*current)->calibrationBegin();
	}

	// flush video buffers
	this->capture.stop();
	this->capture.start();

	return true;
}


bool Processor::addCalibrationListener( Processor::ACalibrationListener * listener )
{
	return this->pImpl->calibrationListeners.insert( listener ).second;
}


bool Processor::removeCalibrationListener( Processor::ACalibrationListener * listener )
{
	return this->pImpl->calibrationListeners.erase( listener );
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
