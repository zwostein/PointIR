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

#ifndef _POINTIR_POINT__INCLUDED_
#define _POINTIR_POINT__INCLUDED_


#if __cplusplus
	#include <cmath>
#endif


typedef float PointIR_Point_Component;


struct PointIR_Point
{
	PointIR_Point_Component x;
	PointIR_Point_Component y;

#if __cplusplus
	typedef PointIR_Point_Component Component;

	/// Initializes all components to their default value.
	inline PointIR_Point() : x(0), y(0) {}

	/// Initializes all components to the given values.
	template<class U> inline PointIR_Point( U _x, U _y ) : x(_x), y(_y) {}

	/// Initializes all components to the values of another point.
	template<class U> inline PointIR_Point( const PointIR_Point & other ) : x(other.x), y(other.y) {}

	/// Assign the coordinates from another point to this one.
	inline PointIR_Point & operator=( const PointIR_Point & rhs )
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	/// Negates each component.
	inline PointIR_Point operator-() const
	{
		return PointIR_Point(-x,-y);
	}

	/// Component addition.
	inline const PointIR_Point operator+( const PointIR_Point & rhs ) const
	{
		return PointIR_Point( x+rhs.x, y+rhs.y );
	}

	/// Calculates difference between two points.
	inline const PointIR_Point operator-( const PointIR_Point & rhs ) const
	{
		return PointIR_Point( x-rhs.x, y-rhs.y );
	}

	/// Scalar multiplication.
	inline const PointIR_Point operator*( const Component & rhs ) const
	{
		return PointIR_Point( x*rhs, y*rhs );
	}

	/// Scalar division.
	inline const PointIR_Point operator/( const Component & rhs ) const
	{
		return PointIR_Point( x/rhs, y/rhs );
	}

	/// Add another point's components.
	inline PointIR_Point & operator+=(const PointIR_Point & rhs )
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	/// Subtract another point's components.
	inline PointIR_Point & operator-=( const PointIR_Point & rhs )
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	/// Multiply by scalar.
	inline PointIR_Point & operator*=( const Component & rhs )
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}

	/// Divide by scalar.
	inline PointIR_Point & operator/=( const Component & rhs )
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}

	/// The distance to another point
	inline PointIR_Point_Component distance( const PointIR_Point & other ) const
	{
		PointIR_Point d = other - *this;
		return std::sqrt( d.x * d.x + d.y * d.y );
	}

	/// The squared distance to another point (faster than distance)
	inline PointIR_Point_Component squaredDistance( const PointIR_Point & other ) const
	{
		PointIR_Point d = other - *this;
		return d.x * d.x + d.y * d.y;
	}

	explicit inline operator bool() const
	{
		return x || y;
	}
#endif
};

typedef struct PointIR_Point PointIR_Point;


#if __cplusplus
namespace PointIR
{
	typedef PointIR_Point Point;
}
#endif


#endif
