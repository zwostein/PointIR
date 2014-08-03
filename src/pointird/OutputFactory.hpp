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

#ifndef _OUTPUTADDER__INCLUDED_
#define _OUTPUTADDER__INCLUDED_


#include <memory>
#include <string>
#include <vector>


class Processor;
class APointOutput;
class AFrameOutput;


class OutputFactory
{
public:
	OutputFactory();
	~OutputFactory();

	APointOutput * newPointOutput( const std::string name ) const;
	AFrameOutput * newFrameOutput( const std::string name ) const;

	std::vector< std::string > getAvailablePointOutputs() const;
	std::vector< std::string > getAvailableFrameOutputs() const;
	std::vector< std::string > getAvailableOutputs() const;

	void setProcessor( const Processor * processor ) { this->processor = processor; }

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;

	const Processor * processor;
};


#endif
