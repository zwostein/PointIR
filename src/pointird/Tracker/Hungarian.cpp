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

#include "Hungarian.hpp"

#include <PointIR/Point.h>
#include <PointIR/PointArray.h>

#include <vector>
#include <set>
#include <algorithm>
#include <limits>


#define MAXPOINTS 32


// some code borrowed from mtdev
void ixoptimal( int * ix, int * mdist, int nrows, int ncols );

static inline int clamp15(int x)
{
	return x < -32767 ? -32767 : x > 32767 ? 32767 : x;
}

// absolute scale is assumed to fit in 15 bits - float scale is usually 0.0 < d < 1.0
static inline int toDist2( float dx, float dy )
{
	dx = clamp15( dx * 32767 );
	dy = clamp15( dy * 32767 );
	return dx * dx + dy * dy;
}


using namespace Tracker;


class Hungarian::Impl
{
public:
	std::set< int > usedIDs;
	unsigned int maxID = MAXPOINTS-1;

	int getFreeID()
	{
		int free = 0;
		// iterate over (sorted) set and use the first gap
		for( auto id : usedIDs )
		{
			if( free != id )
				break;
			++free;
		}
		if( free >= 0 && free <= (int)maxID )
		{
			usedIDs.insert( free );
			return free;
		}
		else
			return -1;
	}

	void setFreeID( int id )
	{
		usedIDs.erase( id );
	}
};


Hungarian::Hungarian() : pImpl(new Impl)
{
}


Hungarian::Hungarian( unsigned int maxID ) : pImpl(new Impl)
{
	if( maxID < MAXPOINTS )
		this->pImpl->maxID = maxID;
}


Hungarian::~Hungarian()
{
}


unsigned int Hungarian::getMaxID() const
{
	return this->pImpl->maxID;
}


void Hungarian::assignIDs( const PointIR::PointArray & previousPoints, const std::vector<int> & previousIDs,
                           const PointIR::PointArray & currentPoints, std::vector<int> & currentIDs,
                           std::vector<int> & previousToCurrent, std::vector<int> & currentToPrevious )
{
	int A[MAXPOINTS*MAXPOINTS];

	int rows = currentPoints.size() > MAXPOINTS ? MAXPOINTS : currentPoints.size();
	int cols = previousPoints.size() > MAXPOINTS ? MAXPOINTS : previousPoints.size();

	// setup distance matrix for contact matching
	for( int j = 0; j < rows; j++ )
	{
		int * row = A + cols * j;
		for( int i = 0; i < cols; i++ )
			row[i] = toDist2( currentPoints[i].x - previousPoints[j].x, currentPoints[i].y - previousPoints[j].y );
	}

	// resize and fill unused values
	currentToPrevious.resize( currentPoints.size(), -1 );

	// apply algorithm
	ixoptimal( currentToPrevious.data(), A, rows, cols );

	// assign IDs to new points
	currentIDs.resize( currentPoints.size(), -1 );
	for( unsigned int currentIdx = 0; currentIdx < currentIDs.size(); ++currentIdx )
	{
		if( currentToPrevious[currentIdx] < 0 || (int)previousIDs.size() <= currentToPrevious[currentIdx] )
		{
			currentIDs[currentIdx] = this->pImpl->getFreeID();
		} else {
			currentIDs[currentIdx] = previousIDs[currentToPrevious[currentIdx]];
		}
	}

	// map previous indices to current indices if they still exist and mark IDs unused if they disappeared
	previousToCurrent.resize( previousPoints.size(), -1 );
	for( unsigned int previousIdx = 0; previousIdx < previousPoints.size(); ++previousIdx )
	{
		previousToCurrent[previousIdx] = -1;
		for( unsigned int currentIdx = 0; currentIdx < currentToPrevious.size(); ++currentIdx )
		{
			if( currentToPrevious[currentIdx] == (int)previousIdx )
			{
				previousToCurrent[previousIdx] = currentIdx;
				break;
			}
		}
		if( previousToCurrent[previousIdx] < 0 )
			this->pImpl->setFreeID( previousIDs[previousIdx] );
	}
}
