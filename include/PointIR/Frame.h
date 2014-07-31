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

#ifndef _POINTIR_FRAME__INCLUDED_
#define _POINTIR_FRAME__INCLUDED_


#include <stdint.h>


#if ( __cplusplus && __GNUC__ )
	//HACK: flexible array members (array[]) are part of C99 but not C++11 and below - however they work with g++, so just disable the warning
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
#endif

typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t data[];
} PointIR_Frame;

#if ( __cplusplus && __GNUC__ )
	#pragma GCC diagnostic pop
#endif


#endif
