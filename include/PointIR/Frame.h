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


//HACK: flexible array members (array[]) are part of C99 but not C++11 and below - however they work with g++, so just disable the warning
#if ( __cplusplus && __GNUC__ )
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-pedantic"
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


#if __cplusplus

#include <stdlib.h>
#include <new>
#include <type_traits>
#include <cstddef>

namespace PointIR
{
	class Frame
	{
	public:
		typedef decltype(PointIR_Frame::width) WidthType;
		typedef decltype(PointIR_Frame::height) HeightType;

		Frame( const Frame & ) = delete; // disable copy constructor

		Frame()
		{
			frame = (PointIR_Frame*) calloc( 1, sizeof(PointIR_Frame) );
			if( !frame )
				throw std::bad_alloc();
		}

		~Frame()
		{
			free( frame );
		}

		WidthType       getWidth()  const noexcept { return frame->width; }
		HeightType      getHeight() const noexcept { return frame->height; }
		const uint8_t * getData()   const noexcept { return frame->data; }
		uint8_t *       getData()         noexcept { return frame->data; }

		explicit operator const PointIR_Frame*() const noexcept { return frame; }

		void resize( WidthType newWidth, HeightType newHeight )
		{
			if( (newWidth != frame->width) || (newHeight != frame->height) )
			{
				frame = (PointIR_Frame*) realloc( frame, sizeof(PointIR_Frame) + newWidth * newHeight );
				if( !frame )
					throw std::bad_alloc();
				frame->width = newWidth;
				frame->height = newHeight;
			}
		}


		////////////////////////////////////////////////////////////////
		// STL compatibility

		typedef decltype(*PointIR_Frame::data)                  reference;
		typedef typename std::remove_reference<reference>::type value_type;
		typedef const value_type &                              const_reference;
		typedef value_type *                                    pointer;
		typedef const value_type *                              const_pointer;
		typedef std::ptrdiff_t                                  difference_type;
		typedef std::size_t                                     size_type;

		class const_iterator;
		class iterator
		{
			friend const_iterator;
		public:
			typedef typename Frame::value_type      value_type;
			typedef typename Frame::reference       reference;
			typedef typename Frame::pointer         pointer;
			typedef typename Frame::size_type       size_type;
			typedef typename Frame::difference_type difference_type;
			//TODO: implement remaining operations (http://www.cplusplus.com/reference/iterator/RandomAccessIterator/)
			//typedef std::random_access_iterator_tag iterator_category;

			iterator( pointer initial )                           noexcept { current = initial; }
			iterator( const iterator & other )                    noexcept { current = other.current; }
			iterator & operator++()                               noexcept { ++current; return *this; }
			iterator & operator= ( const iterator & other )       noexcept { current = other.current; return *this; }
			bool       operator==( const iterator & other ) const noexcept { return current == other.current; }
			bool       operator!=( const iterator & other ) const noexcept { return current != other.current; }
			reference  operator* ()                         const noexcept { return *current; }
			pointer    operator->()                         const noexcept { return current; }
		private:
			pointer current;
		};

		class const_iterator
		{
		public:
			typedef typename Frame::value_type      value_type;
			typedef typename Frame::const_reference const_reference;
			typedef typename Frame::const_pointer   const_pointer;
			typedef typename Frame::size_type       size_type;
			typedef typename Frame::difference_type difference_type;
			//TODO: implement remaining operations (http://www.cplusplus.com/reference/iterator/RandomAccessIterator/)
			//typedef std::random_access_iterator_tag iterator_category;

			const_iterator( const_pointer initial )                           noexcept { current = initial; }
			const_iterator( const iterator & other )                          noexcept { current = other.current; }
			const_iterator( const const_iterator & other )                    noexcept { current = other.current; }
			const_iterator & operator++()                                     noexcept { ++current; return *this; }
			const_iterator & operator= ( const const_iterator & other )       noexcept { current = other.current; return *this; }
			bool             operator==( const const_iterator & other ) const noexcept { return current == other.current; }
			bool             operator!=( const const_iterator & other ) const noexcept { return current != other.current; }
			const_reference  operator* ()                               const noexcept { return *current; }
			const_pointer    operator->()                               const noexcept { return current; }
		private:
			const_pointer current;
		};

		bool      empty() const noexcept { return (frame->width == 0) || (frame->height == 0); }
		size_type size()  const noexcept { return frame->width * frame->height; }

		pointer       data()        noexcept { return frame->data; }
		const_pointer data()  const noexcept { return frame->data; }

		iterator       begin()        noexcept { return iterator( frame->data ); }
		const_iterator begin()  const noexcept { return iterator( frame->data ); }
		const_iterator cbegin() const noexcept { return iterator( frame->data ); }
		iterator       end()          noexcept { return iterator( frame->data + size() ); }
		const_iterator end()    const noexcept { return iterator( frame->data + size() ); }
		const_iterator cend()   const noexcept { return iterator( frame->data + size() ); }

		const_reference front() const noexcept { return frame->data[0]; }
		reference       front()       noexcept { return frame->data[0]; }
		const_reference back()  const noexcept { return frame->data[size()-1]; }
		reference       back()        noexcept { return frame->data[size()-1]; }

		const_reference operator[]( size_type i ) const noexcept { return frame->data[i]; }
		reference       operator[]( size_type i )       noexcept { return frame->data[i]; }

		////////////////////////////////////////////////////////////////

	private:
		PointIR_Frame * frame;
	};
}

#endif


#endif
