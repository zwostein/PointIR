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

//#define POINTIR_PROCESSOR_BENCHMARK


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

#ifdef POINTIR_PROCESSOR_BENCHMARK
	#include <unistd.h>
	#include <time.h>

//	#define POINTIR_PROCESSOR_BENCHMARKCLOCKTYPE CLOCK_MONOTONIC
	#ifndef POINTIR_PROCESSOR_BENCHMARKCLOCKTYPE
		#define POINTIR_PROCESSOR_BENCHMARK_CLOCKTYPE CLOCK_PROCESS_CPUTIME_ID
	#endif

	static void timeStart( struct timespec & time )
	{
		clock_gettime( POINTIR_PROCESSOR_BENCHMARK_CLOCKTYPE, &time );
	}
	static void timeStop( const char * name, struct timespec & time )
	{
		struct timespec thisTime = {};
		clock_gettime( POINTIR_PROCESSOR_BENCHMARK_CLOCKTYPE, &thisTime );
		double elapsed_us = (thisTime.tv_sec - time.tv_sec)*1000000.0 + (thisTime.tv_nsec - time.tv_nsec)/1000.0;
		printf( "%s: %f\n", name, elapsed_us );
	}

	#define TIME( variable ) struct timespec variable={}
	#define TIMESTART( time ) timeStart( time )
	#define TIMESTOP( name, time ) timeStop( name, time )
	#define TIMEFRAMEBEGIN() printf( "--------Processor Benchmark Begin--------\n" )
	#define TIMEFRAMEEND() printf( "--------Processor Benchmark End--------\n" )
#else
	#define TIME( variable )
	#define TIMESTART( time )
	#define TIMESTOP( name, time )
	#define TIMEFRAMEBEGIN()
	#define TIMEFRAMEEND()
#endif


class Processor::Impl
{
public:
	Impl( Processor & processor ) : processor(processor) {}

	PointFilter::APointFilter * filter = nullptr;

	std::set< FrameOutput::AFrameOutput * > frameOutputs;
	std::set< PointOutput::APointOutput * > pointOutputs;
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


Processor::Processor( Capture::ACapture & capture, PointDetector::APointDetector & detector, Unprojector::AUnprojector & unprojector ) :
	pImpl( new Impl( *this ) ),
	capture(capture),
	detector(detector),
	unprojector(unprojector)
{
//	this->startCalibration();
}


Processor::~Processor()
{
}


std::set< FrameOutput::AFrameOutput * > Processor::getFrameOutputs()
{
	return this->pImpl->frameOutputs;
}


std::set< PointOutput::APointOutput * > Processor::getPointOutputs()
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

	TIMEFRAMEBEGIN();

	TIME( total );
	TIMESTART( total );

	TIME( advanceFrame );
	TIMESTART( advanceFrame );
	if( !this->capture.advanceFrame( true, 1.0f ) )
	{
		TIMEFRAMEEND();
		std::cerr << "Processor: Could not get next frame.\n";
		return;
	}
	TIMESTOP( "advanceFrame", advanceFrame );
	TIME( retrieveFrame );
	TIMESTART( retrieveFrame );
	if( !this->capture.retrieveFrame( this->frame ) )
	{
		TIMEFRAMEEND();
		std::cerr << "Processor: Could not retrieve frame.\n";
		return;
	}
	TIMESTOP( "retrieveFrame", retrieveFrame );

	TIME( outputFrame );
	TIMESTART( outputFrame );
	if( this->pImpl->frameOutputEnabled )
	{
		for( FrameOutput::AFrameOutput * output : this->pImpl->frameOutputs )
		output->outputFrame( this->frame );
	}
	TIMESTOP( "outputFrame", outputFrame );

	if( this->isCalibrating() )
	{
		TIME( calibration );
		TIMESTART( calibration );
		//TODO: as soon as there are multiple ways for calibrating, move the calibration logic to an external module/class
		if( Unprojector::AAutoUnprojector * autoUnprojector = dynamic_cast<Unprojector::AAutoUnprojector*>( &(this->unprojector) ) )
		{
			bool result = autoUnprojector->calibrate( this->frame );
			this->pImpl->endCalibration( result );
		}
		else
		{
			// no calibration supported
			this->pImpl->endCalibration( false );
		}
		TIMESTOP( "calibration", calibration );

		TIMESTOP( "total (calibration)", total );
	}
	else
	{
		TIME( detectPoints );
		TIMESTART( detectPoints );
		this->detector.detect( this->pointArray, this->frame );
		TIMESTOP( "detectPoints", detectPoints );

		TIME( unprojectPoints );
		TIMESTART( unprojectPoints );
		this->unprojector.unproject( this->pointArray );
		TIMESTOP( "unprojectPoints", unprojectPoints );

		TIME( filterPoints );
		TIMESTART( filterPoints );
		if( this->pImpl->filter )
			this->pImpl->filter->filterPoints( this->pointArray );
		TIMESTOP( "filterPoints", filterPoints );

		TIME( outputPoints );
		TIMESTART( outputPoints );
		if( this->pImpl->pointOutputEnabled )
		{
			for( PointOutput::APointOutput * output : this->pImpl->pointOutputs )
				output->outputPoints( this->pointArray );
		}
		TIMESTOP( "outputPoints", outputPoints );

		TIMESTOP( "total", total );
	}
	TIMEFRAMEEND();
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


bool Processor::addFrameOutput( FrameOutput::AFrameOutput * output )
{
	return this->pImpl->frameOutputs.insert( output ).second;
}


bool Processor::removeFrameOutput( FrameOutput::AFrameOutput * output )
{
	return this->pImpl->frameOutputs.erase( output );
}


bool Processor::addPointOutput( PointOutput::APointOutput * output )
{
	return this->pImpl->pointOutputs.insert( output ).second;
}


bool Processor::removePointOutput( PointOutput::APointOutput * output )
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


void Processor::setPointFilter( PointFilter::APointFilter * pointFilter )
{
	this->pImpl->filter = pointFilter;
}


PointFilter::APointFilter * Processor::getPointFilter() const
{
	return this->pImpl->filter;
}
