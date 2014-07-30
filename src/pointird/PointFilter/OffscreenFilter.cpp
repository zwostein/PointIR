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


template< typename T >
static void erase_unordered( std::vector< T > & v, typename std::vector< T >::size_type index )
{
	std::swap( v[index], v.back() );
	v.pop_back();
}


void OffscreenFilter::filterPoints( std::vector< PointIR_Point > & points ) const
{
	float minMargin = 0.0f - this->tolerance;
	float maxMargin = 1.0f + this->tolerance;
	for( std::vector< PointIR_Point >::size_type i = 0; i < points.size(); )
	{
		if( points[i].x < minMargin || points[i].x >= maxMargin || points[i].y < minMargin || points[i].y >= maxMargin )
			erase_unordered( points, i );
		else
			i++;
	}
}
