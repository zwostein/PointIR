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

#ifndef _TRACKER_HUNGARIAN__INCLUDED_
#define _TRACKER_HUNGARIAN__INCLUDED_


#include <PointIR/PointArray.h>
#include "ATracker.hpp"

#include <memory>
#include <vector>


namespace Tracker
{

class Hungarian : public ATracker
{
public:
	Hungarian( const Hungarian & ) = delete; // disable copy constructor
	Hungarian & operator=( const Hungarian & other ) = delete; // disable assignment operator

	Hungarian();
	Hungarian( unsigned int maxID );
	~Hungarian();

	void assignIDs( const PointIR::PointArray & previousPoints, const std::vector<int> & previousIDs,
	                const PointIR::PointArray & currentPoints, std::vector<int> & currentIDs,
	                std::vector<int> & previousToCurrent, std::vector<int> & currentToPrevious ) override;

	unsigned int getMaxID() const override;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};

}


#endif
