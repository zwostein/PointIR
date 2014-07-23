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


#include <stdint.h>

#include <memory>
#include <vector>
#include <functional>


class ACapture;
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

	bool addOutput( APointOutput * output );
	bool removeOutput( APointOutput * output );

	void setOutputEnabled( bool enable );
	bool isOutputEnabled() const;

	void setPointFilter( APointFilter * pointFilter );
	APointFilter * getPointFilter() const;

	const std::vector< uint8_t > & getProcessedFrame() const;
	unsigned int getFrameWidth() const;
	unsigned int getFrameHeight() const;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;

	ACapture & capture;
	APointDetector & detector;
	AUnprojector & unprojector;
};


#endif
