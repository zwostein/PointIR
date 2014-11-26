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

#include "CalibrationDataFile.hpp"
#include "AUnprojector.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>


using namespace Unprojector;


CalibrationDataFile::CalibrationDataFile( AUnprojector & unprojector, const std::string & directory ) : unprojector(unprojector)
{
	std::stringstream ss;
	if( !directory.empty() )
	{
		ss << directory;
		if( directory.back() != '/' )
			ss << '/';
	}
	ss << "PointIR.calib";
	this->filename = ss.str();
}


CalibrationDataFile::~CalibrationDataFile()
{
}


bool CalibrationDataFile::load()
{
	std::cout << "CalibrationDataFile: Loading calibration data from \"" + this->filename + "\" ... ";
	std::ifstream file( this->filename, std::ios::in | std::ifstream::binary );
	std::vector< uint8_t > rawData;
	std::copy( std::istreambuf_iterator< char >( file ), std::istreambuf_iterator< char >(), std::back_inserter(rawData) );
	if( this->unprojector.setRawCalibrationData( rawData ) )
	{
		std::cout << "ok" << std::endl;
		return true;
	}
	else
	{
		std::cout << "failed" << std::endl;
		return false;
	}
}


bool CalibrationDataFile::save() const
{
	std::cout << "CalibrationDataFile: Saving calibration data to \"" + this->filename + "\"" << std::endl;
	std::ofstream file( this->filename, std::ios::out | std::ofstream::binary );
	std::vector< uint8_t > rawData = this->unprojector.getRawCalibrationData();
	std::copy( rawData.begin(), rawData.end(), std::ostreambuf_iterator< char >(file) );
	return true;
}
