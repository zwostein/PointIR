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
#include "../exceptions.hpp"

#include <lodepng.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>


using namespace Unprojector;


// the default directory
std::string CalibrationImageFile::directory;


static bool fileExists( const std::string & name )
{
	struct stat buffer;
	return stat( name.c_str(), &buffer ) == 0;
}


void CalibrationImageFile::setDirectory( const std::string & directory )
{
	CalibrationImageFile::directory = directory;
	if( !CalibrationImageFile::directory.empty() && CalibrationImageFile::directory.back() != '/' )
		CalibrationImageFile::directory += '/';
}


CalibrationImageFile::CalibrationImageFile( AAutoUnprojector & unprojector, unsigned int width, unsigned int height ) :
	unprojector(unprojector), width(width), height(height)
{
	std::stringstream ss;
	ss << CalibrationImageFile::directory << "PointIR." << this->width << "x" << this->height << ".png";
	this->filename = ss.str();
}


CalibrationImageFile::~CalibrationImageFile()
{
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

	PointIR::Frame frame;
	this->unprojector.generateCalibrationImage( frame, this->width, this->height );

	unsigned int error = lodepng::encode( fileName,
		frame.getData(),            // raw data
		this->width, this->height,  // image size
		LodePNGColorType::LCT_GREY, // only greyscale
		8                           // 8 bits per channel
	);

	if( error )
		throw RUNTIME_ERROR( "LodePNG encode error " + std::to_string(error) + ": " + lodepng_error_text(error) );

	return true;
}
