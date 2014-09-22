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

#ifndef _POINTIR_VIDEOSOCKETCLIENT__INCLUDED_
#define _POINTIR_VIDEOSOCKETCLIENT__INCLUDED_


#include <PointIR/Frame.h>

#include <string>


namespace PointIR
{
	class VideoSocketClient
	{
	public:
		VideoSocketClient( const std::string & socketName = "/tmp/PointIR.video.socket" );
		virtual ~VideoSocketClient();
		bool receiveFrame( Frame & frame ) const;
	private:
		int socketFD;
	};
}


#endif
