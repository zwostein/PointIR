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

#ifndef _POINTFILTERCHAIN__INCLUDED_
#define _POINTFILTERCHAIN__INCLUDED_


#include "APointFilter.hpp"

#include <list>


namespace PointFilter
{

class Chain : public APointFilter
{
public:
	Chain() {}
	Chain( std::list< const APointFilter * > & filterChain ) : filterChain(filterChain) {}

	void setFilterChain( std::list< const APointFilter * > & filterChain )
	{
		this->filterChain = filterChain;
	}

	void appendFilter( const APointFilter * filter )
	{
		this->filterChain.push_back( filter );
	}

	void prependFilter( const APointFilter * filter )
	{
		this->filterChain.push_front( filter );
	}

	const std::list< const APointFilter * > & getFilters() const
	{
		return this->filterChain;
	}

	virtual void filterPoints( PointIR::PointArray & pointArray ) const override
	{
		for( const APointFilter * filter : this->filterChain )
			filter->filterPoints( pointArray );
	}

private:
	std::list< const APointFilter * > filterChain;
};

}


#endif
