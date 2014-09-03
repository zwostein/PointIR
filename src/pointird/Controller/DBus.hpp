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

#ifndef _CONTROLLER_DBUS__INCLUDED_
#define _CONTROLLER_DBUS__INCLUDED_


#include "AController.hpp"

#include <memory>


class Processor;


namespace Controller
{

class DBus : public AController
{
public:
	DBus( const DBus & ) = delete; // disable copy constructor
	DBus & operator=( const DBus & other ) = delete; // disable assignment operator

	DBus( Processor & processor );
	~DBus();

	virtual void dispatch() override;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};

}


#endif
