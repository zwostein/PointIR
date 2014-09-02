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

#include "Tracker.hpp"

#include <PointIR/Point.h>
#include <PointIR/PointArray.h>

#include <iostream>
#include <vector>
#include <algorithm>

#include <cassert>


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

	T & operator()( unsigned int x, unsigned int y )
	{
		assert( x < width && y < height );
		return elements[ width * y + x ];
	}

	const T & operator()( unsigned int x, unsigned int y ) const
	{
		assert( x < width && y < height );
		return elements[ width * y + x ];
	}
};


class Tracker::Impl
{
public:
	Matrix< PointIR::Point::Component > distancesCurrentPrevious;
	std::vector< int > bestPreviousMatches;
};


Tracker::Tracker() : pImpl(new Impl)
{
}


Tracker::~Tracker()
{
}


//TODO: maybe use the Hungary Algorithm instead
std::vector< int > & Tracker::assignIDs( const PointIR::PointArray & previousPoints, const std::vector< int > & previousIDs,
                         const PointIR::PointArray & currentPoints, std::vector< int > & currentIDs )
{
	this->pImpl->bestPreviousMatches.resize( currentPoints.size() );
	this->pImpl->distancesCurrentPrevious.resize( currentPoints.size(), previousPoints.size() );
	for( unsigned int currentIdx = 0; currentIdx < currentPoints.size(); ++currentIdx )
	{
		int bestMatchIndex = -1;
		for( unsigned int previousIdx = 0; previousIdx < previousPoints.size(); ++previousIdx )
		{
			PointIR::Point::Component distance = currentPoints[currentIdx].squaredDistance( previousPoints[previousIdx] );
			this->pImpl->distancesCurrentPrevious( currentIdx, previousIdx ) = distance;
			if( bestMatchIndex >= 0 )
			{
				if( distance < this->pImpl->distancesCurrentPrevious( currentIdx, bestMatchIndex ) )
					bestMatchIndex = previousIdx;
			} else {
				bestMatchIndex = previousIdx;
			}
		}
		this->pImpl->bestPreviousMatches[currentIdx] = bestMatchIndex;
	}

	// if two points have the same best match, check which one is closer and treat the other one as new
	for( unsigned int a = 0; a < this->pImpl->bestPreviousMatches.size(); ++a )
	{
		for( unsigned int b = a+1; b < this->pImpl->bestPreviousMatches.size(); ++b )
		{
			if( this->pImpl->bestPreviousMatches[a] < 0 || this->pImpl->bestPreviousMatches[b] < 0 )
				continue;

			if( this->pImpl->bestPreviousMatches[a] != this->pImpl->bestPreviousMatches[b] )
				continue;

			if( this->pImpl->distancesCurrentPrevious( a, this->pImpl->bestPreviousMatches[a] )
			    <= this->pImpl->distancesCurrentPrevious( b, this->pImpl->bestPreviousMatches[b] ) )
			{
				this->pImpl->bestPreviousMatches[b] = -1;
			} else {
				this->pImpl->bestPreviousMatches[a] = -1;
			}
		}
	}

	int nextFreeID = 0;
	if( previousIDs.size() )
		nextFreeID = *std::max_element( previousIDs.begin(), previousIDs.end() ) + 1;

	currentIDs.resize( currentPoints.size() );
	for( unsigned int currentIdx = 0; currentIdx < currentIDs.size(); ++currentIdx )
	{
		if( this->pImpl->bestPreviousMatches[currentIdx] < 0 || (int)previousIDs.size() <= this->pImpl->bestPreviousMatches[currentIdx] )
		{
			currentIDs[currentIdx] = nextFreeID++;
		} else {
			currentIDs[currentIdx] = previousIDs[this->pImpl->bestPreviousMatches[currentIdx]];
		}
	}

	return this->pImpl->bestPreviousMatches;
}
