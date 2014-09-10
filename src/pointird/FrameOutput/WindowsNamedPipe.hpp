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

#ifndef _FRAMEOUTPUT_WINDOWSNAMEDPIPE__INCLUDED_
#define _FRAMEOUTPUT_WINDOWSNAMEDPIPE__INCLUDED_


#include "AFrameOutput.hpp"

#include <string>
#include <memory>


namespace FrameOutput
{


class WindowsNamedPipe : public AFrameOutput
{
public:
	WindowsNamedPipe( const WindowsNamedPipe & ) = delete; // disable copy constructor
	WindowsNamedPipe & operator=( const WindowsNamedPipe & other ) = delete; // disable assignment operator

	WindowsNamedPipe();
	virtual ~WindowsNamedPipe();

	virtual void outputFrame( const PointIR::Frame & frame ) override;

	const std::string & getName() const;

private:
	class Impl;
	std::unique_ptr< Impl > pImpl;
};

}


#endif
