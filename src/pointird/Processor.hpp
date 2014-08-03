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

#ifndef _PROCESSOR__INCLUDED_
#define _PROCESSOR__INCLUDED_


#include <PointIR/Frame.h>
#include <PointIR/PointArray.h>

#include <stdint.h>

#include <memory>
#include <set>
#include <functional>


class ACapture;
class AFrameOutput;
class APointDetector;
class AUnprojector;
class APointFilter;
class APointOutput;


class Processor
{
public:
	Processor( const Processor & ) = delete; // disable copy constructor

	Processor( ACapture & capture, APointDetector & detector, AUnprojector & unprojector );
	~Processor();

	ACapture & getCapture() const { return this->capture; }
	APointDetector & getPointDetector() const { return this->detector; }
	AUnprojector & getUnprojector() const { return this->unprojector; }

	void processFrame();

	void start();
	void stop();
	bool isProcessing() const;

	bool startCalibration( std::function< void(bool) > resultCallback );
	void setCalibrationBeginCallback( std::function< void(void) > beginCallback );
	void setCalibrationEndCallback( std::function< void(void) > endCallback );
	bool isCalibrating() const;
	bool isCalibrationSucceeded() const;

	bool addFrameOutput( AFrameOutput * output );
	bool removeFrameOutput( AFrameOutput * output );
	void setFrameOutputEnabled( bool enable );
	bool isFrameOutputEnabled() const;
	std::set<AFrameOutput*> getFrameOutputs();

	bool addPointOutput( APointOutput * output );
	bool removePointOutput( APointOutput * output );
	void setPointOutputEnabled( bool enable );
	bool isPointOutputEnabled() const;
	std::set<APointOutput*> getPointOutputs();

	void setPointFilter( APointFilter * pointFilter );
	APointFilter * getPointFilter() const;

	const PointIR_Frame * getProcessedFrame() const { return this->frame; }

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;

	ACapture & capture;
	APointDetector & detector;
	AUnprojector & unprojector;

	PointIR::Frame frame;
	PointIR::PointArray pointArray;
};


#endif
