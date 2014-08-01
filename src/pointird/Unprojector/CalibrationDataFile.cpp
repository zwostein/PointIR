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
#include <fstream>
#include <vector>


// the default directory
std::string CalibrationDataFile::directory = "/tmp/";


static std::string getCalibrationDataFileName( const AUnprojector & unprojector )
{
	return CalibrationDataFile::getDirectory() + std::string("PointIR.calib");
}


void CalibrationDataFile::setDirectory( const std::string & directory )
{
	CalibrationDataFile::directory = directory;
	if( !CalibrationDataFile::directory.empty() && CalibrationDataFile::directory.back() != '/' )
		CalibrationDataFile::directory += '/';
}


CalibrationDataFile::CalibrationDataFile( AUnprojector & unprojector ) : unprojector(unprojector)
{
}


CalibrationDataFile::~CalibrationDataFile()
{
}


bool CalibrationDataFile::load( AUnprojector & unprojector )
{
	std::string fileName = getCalibrationDataFileName( unprojector );
	std::cout << "CalibrationDataFile: Loading calibration data from \"" + fileName + "\" ... ";
	std::ifstream file( fileName, std::ios::in | std::ifstream::binary );
	std::vector< uint8_t > rawData;
	std::copy( std::istreambuf_iterator< char >( file ), std::istreambuf_iterator< char >(), std::back_inserter(rawData) );
	if( unprojector.setRawCalibrationData( rawData ) )
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


bool CalibrationDataFile::save( const AUnprojector & unprojector )
{
	std::string fileName = getCalibrationDataFileName( unprojector );
	std::cout << "CalibrationDataFile: Saving calibration data to \"" + fileName + "\"" << std::endl;
	std::ofstream file( fileName, std::ios::out | std::ofstream::binary );
	std::vector< uint8_t > rawData = unprojector.getRawCalibrationData();
	std::copy( rawData.begin(), rawData.end(), std::ostreambuf_iterator< char >(file) );
	return true;
}
