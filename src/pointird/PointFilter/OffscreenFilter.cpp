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

#include "OffscreenFilter.hpp"

#include <PointIR/PointArray.h>


template< typename T >
static void erase_unordered( T & v, size_t index )
{
	std::swap( v[index], v.back() );
	v.pop_back();
}


void OffscreenFilter::filterPoints( PointIR::PointArray & pointArray ) const
{
	float minMargin = 0.0f - this->tolerance;
	float maxMargin = 1.0f + this->tolerance;
	for( PointIR::PointArray::size_type i = 0; i < pointArray.size(); )
	{
		if( pointArray[i].x < minMargin || pointArray[i].x >= maxMargin || pointArray[i].y < minMargin || pointArray[i].y >= maxMargin )
			erase_unordered( pointArray, i );
		else
			i++;
	}
}
