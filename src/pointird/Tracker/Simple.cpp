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

#include "Simple.hpp"

#include <PointIR/Point.h>
#include <PointIR/PointArray.h>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>

#include <cassert>


using namespace Tracker;


template <typename T>
class Matrix
{
private:
	std::vector<T> elements;
	unsigned int width = 0;
	unsigned int height = 0;

public:
	Matrix()
	{
	}

	Matrix( unsigned int width, unsigned int height ) : width(width), height(height)
	{
		this->elements.resize( width * height );
	}

	void resize( unsigned int width, unsigned int height )
	{
		this->width = width;
		this->height = height;
		this->elements.resize( width * height );
	}

	inline T & operator()( unsigned int x, unsigned int y )
	{
		assert( x < width && y < height );
		return elements[ width * y + x ];
	}

	inline const T & operator()( unsigned int x, unsigned int y ) const
	{
		assert( x < width && y < height );
		return elements[ width * y + x ];
	}
};


class Simple::Impl
{
public:
	Matrix< PointIR::Point::Component > distancesCurrentPrevious;

	std::set< int > usedIDs;
	unsigned int maxID = std::numeric_limits<int>::max();

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


Simple::Simple() : pImpl(new Impl)
{
}


Simple::Simple( unsigned int maxID ) : pImpl(new Impl)
{
	if( maxID <= (unsigned int)std::numeric_limits<int>::max() )
		this->pImpl->maxID = maxID;
}


Simple::~Simple()
{
}


unsigned int Simple::getMaxID() const
{
	return this->pImpl->maxID;
}


//TODO: maybe use the Hungary Algorithm instead
void Simple::assignIDs( const PointIR::PointArray & previousPoints, const std::vector<int> & previousIDs,
                        const PointIR::PointArray & currentPoints, std::vector<int> & currentIDs,
                        std::vector<int> & previousToCurrent, std::vector<int> & currentToPrevious )
{
	// build distance matrix and mark best matches for each point
	currentToPrevious.resize( currentPoints.size(), -1 );
	Matrix< PointIR::Point::Component > & matrix = this->pImpl->distancesCurrentPrevious;
	matrix.resize( currentPoints.size(), previousPoints.size() );
	for( unsigned int currentIdx = 0; currentIdx < currentPoints.size(); ++currentIdx )
	{
		int bestMatchIndex = -1;
		for( unsigned int previousIdx = 0; previousIdx < previousPoints.size(); ++previousIdx )
		{
			PointIR::Point::Component distance = currentPoints[currentIdx].squaredDistance( previousPoints[previousIdx] );
			matrix( currentIdx, previousIdx ) = distance;
			if( bestMatchIndex >= 0 )
			{
				if( distance < matrix( currentIdx, bestMatchIndex ) )
					bestMatchIndex = previousIdx;
			} else {
				bestMatchIndex = previousIdx;
			}
		}
		currentToPrevious[currentIdx] = bestMatchIndex;
	}

	// if two points have the same best match, check which one is closer and treat the other one as new
	for( unsigned int a = 0; a < currentToPrevious.size(); ++a )
	{
		for( unsigned int b = a+1; b < currentToPrevious.size(); ++b )
		{
			if( currentToPrevious[a] < 0 || currentToPrevious[b] < 0 )
				continue; // at least one of the points doesn't have a match at all

			if( currentToPrevious[a] != currentToPrevious[b] )
				continue; // the two points do not have the same match

			if( this->pImpl->distancesCurrentPrevious( a, currentToPrevious[a] )
			 <= this->pImpl->distancesCurrentPrevious( b, currentToPrevious[b] ) )
			{
				currentToPrevious[b] = -1;
			} else {
				currentToPrevious[a] = -1;
			}
		}
	}

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
