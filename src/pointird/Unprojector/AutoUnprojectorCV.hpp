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

#ifndef _AUTOUNPROJECTORCV__INCLUDED_
#define _AUTOUNPROJECTORCV__INCLUDED_


#include "AAutoUnprojector.hpp"

#include <PointIR/Point.h>

#include <memory>

#include <stdint.h>


class AutoUnprojectorCV : public AAutoUnprojector
{
public:
	AutoUnprojectorCV();
	~AutoUnprojectorCV();

	virtual void unproject( uint8_t * greyImage, unsigned int width, unsigned int height ) const override;
	virtual void unproject( std::vector< PointIR_Point > & points ) const override;

	virtual std::vector< uint8_t > getRawCalibrationData() const override;
	virtual bool setRawCalibrationData( const std::vector< uint8_t > & data ) override;

	virtual bool calibrate( const uint8_t * greyImage, unsigned int width, unsigned int height ) override;
	virtual void generateCalibrationImage( uint8_t * greyImage, unsigned int width, unsigned int height ) const override;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};


#endif
