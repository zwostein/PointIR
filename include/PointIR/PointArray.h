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

#ifndef _POINTIR_POINTARRAY__INCLUDED_
#define _POINTIR_POINTARRAY__INCLUDED_


#include <PointIR/Point.h>
#include <stdint.h>


//HACK: flexible array members (array[]) are part of C99 but not C++11 and below - however they work with g++, so just disable the warning
#if ( __cplusplus && __GNUC__ )
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-pedantic"
#endif

typedef struct
{
	uint32_t count;
	PointIR_Point points[];
} PointIR_PointArray;

#if ( __cplusplus && __GNUC__ )
	#pragma GCC diagnostic pop
#endif


#if __cplusplus

#include <stdlib.h>
#include <new>
#include <cstddef>
#include <cstring>

namespace PointIR
{
	class PointArray
	{
	public:
		typedef decltype(PointIR_PointArray::count) CountType;

		PointArray()
		{
			pointArray = (PointIR_PointArray*) calloc( 1, sizeof(PointIR_PointArray) );
			if( !pointArray )
				throw std::bad_alloc();
			pointArrayCapacity = 0;
		}

		~PointArray()
		{
			free( pointArray );
		}

		PointArray( const PointArray & ) = delete; // disable copy constructor

		PointArray & operator=( const PointArray & other )
		{
			this->resize( other.size() );
			memcpy( this->pointArray, other.pointArray, sizeInBytes( other.size() ) );
			return *this;
		}

		CountType     getCount()  const noexcept { return pointArray->count; }
		const Point * getPoints() const noexcept { return pointArray->points; }
		Point *       getPoints()       noexcept { return pointArray->points; }

		explicit operator const PointIR_PointArray*() const noexcept { return pointArray; }

		void resizeIfNeeded( CountType newCount )
		{
			if( newCount > pointArrayCapacity )
				_resize( newCount );
			pointArray->count = pointArrayCapacity;
		}


		////////////////////////////////////////////////////////////////
		// STL compatibility

		typedef Point &                             reference;
		typedef Point                               value_type;
		typedef const value_type &                  const_reference;
		typedef value_type *                        pointer;
		typedef const value_type *                  const_pointer;
		typedef std::ptrdiff_t                      difference_type;
		typedef decltype(PointIR_PointArray::count) size_type;

		class const_iterator;
		class iterator
		{
			friend const_iterator;
		public:
			typedef typename PointArray::value_type      value_type;
			typedef typename PointArray::reference       reference;
			typedef typename PointArray::pointer         pointer;
			typedef typename PointArray::size_type       size_type;
			typedef typename PointArray::difference_type difference_type;
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
			typedef typename PointArray::value_type      value_type;
			typedef typename PointArray::const_reference const_reference;
			typedef typename PointArray::const_pointer   const_pointer;
			typedef typename PointArray::size_type       size_type;
			typedef typename PointArray::difference_type difference_type;
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

		void resize( size_type newCount )
		{
			if( newCount != pointArrayCapacity )
				_resize( newCount );
			pointArray->count = pointArrayCapacity;
		}

		bool empty()         const noexcept { return pointArray->count == 0; }
		size_type size()     const noexcept { return pointArray->count; }
		size_type capacity() const noexcept { return pointArrayCapacity; }

		pointer       data()       noexcept { return pointArray->points; }
		const_pointer data() const noexcept { return pointArray->points; }

		iterator       begin()        noexcept { return iterator( pointArray->points ); }
		const_iterator begin()  const noexcept { return iterator( pointArray->points ); }
		const_iterator cbegin() const noexcept { return iterator( pointArray->points ); }
		iterator       end()          noexcept { return iterator( pointArray->points + pointArray->count ); }
		const_iterator end()    const noexcept { return iterator( pointArray->points + pointArray->count ); }
		const_iterator cend()   const noexcept { return iterator( pointArray->points + pointArray->count ); }

		const_reference front() const noexcept { return pointArray->points[0]; }
		reference       front()       noexcept { return pointArray->points[0]; }
		const_reference back()  const noexcept { return pointArray->points[pointArray->count-1]; }
		reference       back()        noexcept { return pointArray->points[pointArray->count-1]; }

		void pop_back()    noexcept { --(pointArray->count); }

		const_reference operator[]( size_type i ) const noexcept { return pointArray->points[i]; }
		reference       operator[]( size_type i )       noexcept { return pointArray->points[i]; }

		////////////////////////////////////////////////////////////////


	private:
		static size_t sizeInBytes( CountType size ) { return sizeof(PointIR_PointArray) + size * sizeof(PointIR_Point); }

		void _resize( CountType newCapacity )
		{
			pointArray = (PointIR_PointArray*) realloc( pointArray, sizeInBytes( newCapacity ) );
			if( !pointArray )
				throw std::bad_alloc();
			pointArrayCapacity = newCapacity;
		}

		CountType pointArrayCapacity;
		PointIR_PointArray * pointArray;
	};
}

#endif


#endif
