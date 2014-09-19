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


namespace Capture
{
	class ACapture;
}

namespace FrameOutput
{
	class AFrameOutput;
}

namespace PointDetector
{
	class APointDetector;
}

namespace Unprojector
{
	class AUnprojector;
}

namespace PointFilter
{
	class APointFilter;
}

namespace PointOutput
{
	class APointOutput;
}


class Processor
{
public:
	class ACalibrationListener
	{
	public:
		virtual void calibrationBegin() {}
		virtual void calibrationEnd( bool success ) {}
	};

	Processor( const Processor & ) = delete; // disable copy constructor

	Processor( Capture::ACapture & capture, PointDetector::APointDetector & detector, Unprojector::AUnprojector & unprojector );
	~Processor();

	Capture::ACapture & getCapture() const { return this->capture; }
	PointDetector::APointDetector & getPointDetector() const { return this->detector; }
	Unprojector::AUnprojector & getUnprojector() const { return this->unprojector; }

	void processFrame();

	void start();
	void stop();
	bool isProcessing() const;

	bool startCalibration();
	bool addCalibrationListener( ACalibrationListener * listener );
	bool removeCalibrationListener( ACalibrationListener * listener );
	bool isCalibrating() const;
	bool isCalibrationSucceeded() const;

	bool addFrameOutput( FrameOutput::AFrameOutput * output );
	bool removeFrameOutput( FrameOutput::AFrameOutput * output );
	void setFrameOutputEnabled( bool enable );
	bool isFrameOutputEnabled() const;
	std::set<FrameOutput::AFrameOutput*> getFrameOutputs();

	bool addPointOutput( PointOutput::APointOutput * output );
	bool removePointOutput( PointOutput::APointOutput * output );
	void setPointOutputEnabled( bool enable );
	bool isPointOutputEnabled() const;
	std::set<PointOutput::APointOutput*> getPointOutputs();

	void setPointFilter( PointFilter::APointFilter * pointFilter );
	PointFilter::APointFilter * getPointFilter() const;

	const PointIR_Frame * getProcessedFrame() const { return (const PointIR_Frame *)(this->frame); }

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;

	Capture::ACapture & capture;
	PointDetector::APointDetector & detector;
	Unprojector::AUnprojector & unprojector;

	PointIR::Frame frame;
	PointIR::PointArray pointArray;
};


#endif
