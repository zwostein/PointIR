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

#include "CalibrationImageFile.hpp"

#include "AAutoUnprojector.hpp"

#include <lodepng.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>


#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


static bool fileExists( const std::string & name )
{
	struct stat buffer;
	return stat( name.c_str(), &buffer ) == 0;
}


CalibrationImageFile::CalibrationImageFile( AAutoUnprojector & unprojector, unsigned int width, unsigned int height ) :
	unprojector(unprojector), width(width), height(height)
{
}


CalibrationImageFile::~CalibrationImageFile()
{
}


std::string CalibrationImageFile::getFilename() const
{
	std::stringstream ss;
//	ss << "/tmp/PointIR." << getpid() << "." << this->width << "x" << this->height << ".png";
	ss << "/tmp/PointIR." << this->width << "x" << this->height << ".png";
	return ss.str();
}


bool CalibrationImageFile::generate()
{
	std::string fileName = this->getFilename();

	if( fileExists( fileName ) )
	{
		std::cout << "CalibrationImageFile: Calibration image \"" + fileName + "\" already existing" << std::endl;
		return false;
	}

	std::cout << "CalibrationImageFile: Saving calibration image to \"" + fileName + "\"" << std::endl;

	uint8_t * greyImage = new uint8_t[ this->width * this->height ];
	this->unprojector.generateCalibrationImage( greyImage, this->width, this->height );

	unsigned int error = lodepng::encode( fileName,
		greyImage,                  // raw data
		this->width, this->height,  // image size
		LodePNGColorType::LCT_GREY, // only greyscale
		8                           // 8 bits per channel
	);

	if( error )
		throw RUNTIME_ERROR( "LodePNG encode error " + std::to_string(error) + ": " + lodepng_error_text(error) );

	return true;
}
