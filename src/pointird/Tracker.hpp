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

#ifndef _TRACKER__INCLUDED_
#define _TRACKER__INCLUDED_


#include <PointIR/PointArray.h>

#include <memory>
#include <vector>


class Tracker
{
public:
	Tracker( const Tracker & ) = delete; // disable copy constructor

	Tracker();
	Tracker( unsigned int maxID );
	~Tracker();

	void assignIDs( const PointIR::PointArray & previousPoints, const std::vector<int> & previousIDs,
	                const PointIR::PointArray & currentPoints, std::vector<int> & currentIDs,
	                std::vector<int> & previousToCurrent, std::vector<int> & currentToPrevious );

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};


#endif
