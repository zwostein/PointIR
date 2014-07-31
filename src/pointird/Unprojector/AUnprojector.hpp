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

#ifndef _AUNPROJECTOR__INCLUDED_
#define _AUNPROJECTOR__INCLUDED_


#include <PointIR/Point.h>
#include <PointIR/Frame.h>

#include <stdint.h>

#include <vector>


class AUnprojector
{
public:
	virtual void unproject( uint8_t * greyImage, unsigned int width, unsigned int height ) const = 0;
	virtual void unproject( std::vector< PointIR_Point > & points ) const = 0;

	virtual std::vector< uint8_t > getRawCalibrationData() const = 0;
	virtual bool setRawCalibrationData( const std::vector< uint8_t > & data ) = 0;
};


#endif
