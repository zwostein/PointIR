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

#ifndef _POINTDETECTORCV__INCLUDED_
#define _POINTDETECTORCV__INCLUDED_


#include "APointDetector.hpp"
#include "../Point.hpp"

#include <vector>

#include <stdint.h>


class PointDetectorCV : public APointDetector
{
public:
	virtual std::vector< Point > detect( const uint8_t * image, unsigned int width, unsigned int height ) override;

	void setIntensityThreshold( uint8_t threshold ) { this->intensityThreshold = threshold; }
	uint8_t getIntensityThreshold() const { return this->intensityThreshold; }

	void setBoundingFilterEnabled( bool enable ) { this->boundingFilterEnabled = enable; }
	bool isBoundingFilterEnabled() const { return this->boundingFilterEnabled; }
	void setMinBoundingSize( const float & minBoundingSize ) { this->minBoundingSize = minBoundingSize; }
	void setMaxBoundingSize( const float & maxBoundingSize ) { this->maxBoundingSize = maxBoundingSize; }
	const float & getMinBoundingSize() const { return this->minBoundingSize; }
	const float & getMaxBoundingSize() const { return this->maxBoundingSize; }

private:
	uint8_t intensityThreshold = 128;
	bool boundingFilterEnabled = false;
	float minBoundingSize = 0.0002f;
	float maxBoundingSize = 0.125f;
};


#endif
