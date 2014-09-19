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

#ifndef _CALIBRATIONDATAFILE__INCLUDED_
#define _CALIBRATIONDATAFILE__INCLUDED_


#include <string>


namespace Unprojector
{

class AUnprojector;


class CalibrationDataFile
{
public:
	CalibrationDataFile( AUnprojector & unprojector );
	~CalibrationDataFile();

	bool load() { return CalibrationDataFile::load( this->unprojector ); }
	bool save() { return CalibrationDataFile::save( this->unprojector ); }

	static bool load( AUnprojector & unprojector );
	static bool save( const AUnprojector & unprojector );

	static void setDirectory( const std::string & directory );
	static const std::string & getDirectory() { return CalibrationDataFile::directory; }

private:
	static std::string directory;
	AUnprojector & unprojector;
};

}


#endif
