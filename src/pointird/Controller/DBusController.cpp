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

#include "DBusController.hpp"

#include "../Processor.hpp"
#include "../Unprojector/AAutoUnprojector.hpp"
#include "../Unprojector/CalibrationDataFile.hpp"
#include "../Unprojector/CalibrationImageFile.hpp"
#include "../PointDetector/PointDetectorCV.hpp"
//#include "../FrameOutput/UnixDomainSocketFrameOutput.hpp"
//#include "../PointOutput/UnixDomainSocketPointOutput.hpp"

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <typeinfo>
#include <stdexcept>

#include <string.h>

#include <dbus/dbus.h>

/*
template< typename T > struct nameof {};
#define NAME_CLASS( classtype ) \
	template<> struct nameof<classtype> { const static std::string value() { return #classtype; } }

NAME_CLASS( UnixDomainSocketPointOutput );
NAME_CLASS( UnixDomainSocketFrameOutput );
*/

#define RUNTIME_ERROR( whattext ) \
	std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define DBUS_ERROR( connection, message, errorName, text ) \
	dbusError( connection, message, errorName, text, std::string(__PRETTY_FUNCTION__) )


static void dbusError( DBusConnection * connection, DBusMessage * message, const char * errorName, const std::string & text, const std::string & where )
{
	std::cerr << where << std::string(": ") << std::string(text) << "\n";
	DBusMessage * reply = dbus_message_new_error( message, errorName, text.c_str() );
	dbus_connection_send( connection, reply, nullptr );
	dbus_message_unref( reply );
}


static const char * dBusName = "PointIR.Controller";


class DBusController::Impl
{
public:
	template< typename T > using Setter = std::function< void( T ) >;
	template< typename T > using Getter = std::function< T( void ) >;
	using Call = std::function< void( void ) >;

	typedef std::function< void( DBusConnection*, DBusMessage* ) > Handler;
	typedef std::map< std::string, Handler > MethodMap;
	typedef std::map< std::string, MethodMap > InterfaceMap;

	Processor & processor;
	CalibrationDataFile calibrationDataFile;
	DBusError error;
	DBusConnection * connection = nullptr;
	InterfaceMap interfaceMap;

	Impl( Processor & processor ) : processor(processor), calibrationDataFile( processor.getUnprojector() )
	{
		// create dynamic dbus interfaces depending on the modules used in the processor
		//TODO: this looks ugly

		using namespace std::placeholders;

		MethodMap unprojectorMethods;
		unprojectorMethods.insert( { "saveCalibrationData", std::bind( &Impl::get< bool >, this, Getter< bool >(
			std::bind( static_cast<bool(CalibrationDataFile::*)(void)>(&CalibrationDataFile::save), &calibrationDataFile ) ),
			_1, _2 ) } );
		unprojectorMethods.insert( { "loadCalibrationData", std::bind( &Impl::get< bool >, this, Getter< bool >(
			std::bind( static_cast<bool(CalibrationDataFile::*)(void)>(&CalibrationDataFile::load), &calibrationDataFile ) ),
			_1, _2 ) } );
		if( dynamic_cast<AAutoUnprojector*>( &(processor.getUnprojector()) ) )
		{
			unprojectorMethods.insert( { "generateCalibrationImageFile",
				std::bind( &Impl::generateCalibrationImageFile, this,
				_1, _2 ) } );
		}
		this->interfaceMap.insert( { "PointIR.Controller.Unprojector", unprojectorMethods } );

		MethodMap pointDetectorMethods;
		if( PointDetectorCV * pointDetector = dynamic_cast<PointDetectorCV*>( &(processor.getPointDetector()) ) )
		{
			pointDetectorMethods.insert( { "setIntensityThreshold", std::bind( &Impl::set< unsigned char >, this,
				Setter< unsigned char >( std::bind( &PointDetectorCV::setIntensityThreshold, pointDetector, _1 ) ),
				_1, _2 ) } );
			pointDetectorMethods.insert( { "getIntensityThreshold", std::bind( &Impl::get< unsigned char >, this,
				Getter< unsigned char >( std::bind( &PointDetectorCV::getIntensityThreshold, pointDetector ) ),
				_1, _2 ) } );
		}
		this->interfaceMap.insert( { "PointIR.Controller.PointDetector", pointDetectorMethods } );

		MethodMap processorMethods;
		processorMethods.insert( { "calibrate",
			std::bind( &Impl::calibrate, this,
			_1, _2 ) } );
		processorMethods.insert( { "start", std::bind( &Impl::call, this,
			Call( std::bind( &Processor::start, &processor ) ),
			_1, _2 ) } );
		processorMethods.insert( { "stop", std::bind( &Impl::call, this,
			Call( std::bind( &Processor::stop, &processor ) ),
			_1, _2 ) } );
		processorMethods.insert( { "isProcessing", std::bind( &Impl::get< bool >, this,
			Getter< bool >( std::bind( &Processor::isProcessing, &processor ) ),
			_1, _2 ) } );
		processorMethods.insert( { "setFrameOutputEnabled", std::bind( &Impl::set< bool >, this,
			Setter< bool >( std::bind( &Processor::setFrameOutputEnabled, &processor, _1 ) ),
			_1, _2 ) } );
		processorMethods.insert( { "isFrameOutputEnabled", std::bind( &Impl::get< bool >, this,
			Getter< bool >( std::bind( &Processor::isFrameOutputEnabled, &processor ) ),
			_1, _2 ) } );
		processorMethods.insert( { "setPointOutputEnabled", std::bind( &Impl::set< bool >, this,
			Setter< bool >( std::bind( &Processor::setPointOutputEnabled, &processor, _1 ) ),
			_1, _2 ) } );
		processorMethods.insert( { "isPointOutputEnabled", std::bind( &Impl::get< bool >, this,
			Getter< bool >( std::bind( &Processor::isPointOutputEnabled, &processor ) ),
			_1, _2 ) } );
		this->interfaceMap.insert( { "PointIR.Controller.Processor", processorMethods } );
	}


	void generateCalibrationImageFile( DBusConnection * connection, DBusMessage * message )
	{
		DBusBasicValue value;
		// process message arguments
		DBusMessageIter args;
		if( !dbus_message_iter_init( message, &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes two arguments" );
		if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_UINT32 )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "First argument must be of type uint32" );
		dbus_message_iter_get_basic( &args, &value );
		unsigned int width = value.u32;

		if( !dbus_message_iter_next( &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes two arguments" );
		if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_UINT32 )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "Second argument must be of type uint32" );
		dbus_message_iter_get_basic( &args, &value );
		unsigned int height = value.u32;

		// execute method
		AAutoUnprojector & autoUnprojector = dynamic_cast<AAutoUnprojector&>( this->processor.getUnprojector() );
		CalibrationImageFile calibrationImageFile( autoUnprojector, width, height );
		calibrationImageFile.generate();

		std::string fileName = calibrationImageFile.getFilename();
		value.str = const_cast<char*>(fileName.c_str());

		// generate reply message
		DBusMessage * reply = dbus_message_new_method_return ( message );
		dbus_message_iter_init_append( reply, &args );
		if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &value ) )
			throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );

		// send reply message
		if( !dbus_connection_send( connection, reply, nullptr ) )
			throw RUNTIME_ERROR( "dbus_connection_send failed" );
		dbus_message_unref( reply );
	}


	void calibrationResultCallback( bool result, DBusConnection * connection, DBusMessage * message )
	{
		DBusMessageIter args;

		// dbus_message_iter_append_basic needs this or it will read uninitialized data! (sizeof dbus_bool_t > bool)
		dbus_bool_t ret = result;

		// generate reply message
		DBusMessage * reply = dbus_message_new_method_return ( message );
		dbus_message_iter_init_append( reply, &args );
		if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_BOOLEAN, &ret ) )
			throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );

		// send reply message
		if( !dbus_connection_send( connection, reply, nullptr ) )
			throw RUNTIME_ERROR( "dbus_connection_send failed" );
		dbus_message_unref( reply );

		// clean up message
		dbus_message_unref( message );
	}


	void calibrate( DBusConnection * connection, DBusMessage * message )
	{
		// process message arguments
		DBusMessageIter args;
		if( dbus_message_iter_init( message, &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes no arguments" );

		// keep this message until callback is received
		dbus_message_ref( message );

		// execute method
		this->processor.startCalibration( std::bind( &Impl::calibrationResultCallback, this, std::placeholders::_1, connection, message ) );
	}


	void call( Call method, DBusConnection * connection, DBusMessage * message )
	{
		// process message arguments
		DBusMessageIter args;
		if( dbus_message_iter_init( message, &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes no arguments" );

		// execute method
		method();

		// generate reply message
		DBusMessage * reply = dbus_message_new_method_return ( message );

		// send reply message
		if( !dbus_connection_send( connection, reply, nullptr ) )
			throw RUNTIME_ERROR( "dbus_connection_send failed" );
		dbus_message_unref( reply );
	}


	template< typename TType > struct DBusType { constexpr static const int type = DBUS_TYPE_INVALID; };


	template< typename TType >
	void set( Setter< TType > setter, DBusConnection * connection, DBusMessage * message )
	{
		static_assert( DBusType<TType>::type != DBUS_TYPE_INVALID, "Unknown conversion to DBus type" );

		// process message arguments
		DBusMessageIter args;
		if( !dbus_message_iter_init( message, &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes one argument" );
		if( dbus_message_iter_get_arg_type( &args ) != DBusType<TType>::type )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, std::string("This function takes type \"") + static_cast<char>(DBusType<TType>::type) + "\" as argument" );

		// retrieve argument
		DBusBasicValue value;
		dbus_message_iter_get_basic( &args, &value );

		// execute method
		setter( reinterpret_cast< TType & >( value ) );

		// generate reply message
		DBusMessage * reply = dbus_message_new_method_return ( message );

		// send reply message
		if( !dbus_connection_send( connection, reply, nullptr ) )
			throw RUNTIME_ERROR( "dbus_connection_send failed" );
		dbus_message_unref( reply );
	}


	template< typename TType >
	void get( Getter< TType > getter, DBusConnection * connection, DBusMessage * message )
	{
		static_assert( DBusType<TType>::type != DBUS_TYPE_INVALID, "Unknown conversion to DBus type" );

		// process message arguments
		DBusMessageIter args;
		if( dbus_message_iter_init( message, &args ) )
			return DBUS_ERROR( connection, message, DBUS_ERROR_INVALID_ARGS, "This function takes no arguments" );

		// prepare returned value
		DBusBasicValue value;
		memset( &value, 0, sizeof( value ) );

		// execute method
		reinterpret_cast< TType & >( value ) = getter();

		// generate reply message
		DBusMessage * reply = dbus_message_new_method_return ( message );
		dbus_message_iter_init_append( reply, &args );
		if( !dbus_message_iter_append_basic( &args, DBusType<TType>::type, &value ) )
			throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );

		// send reply message
		if( !dbus_connection_send( connection, reply, nullptr ) )
			throw RUNTIME_ERROR( "dbus_connection_send failed" );
		dbus_message_unref( reply );
	}
};


template<> struct DBusController::Impl::DBusType< unsigned char > { constexpr static const int type = DBUS_TYPE_BYTE; };
template<> struct DBusController::Impl::DBusType< bool >          { constexpr static const int type = DBUS_TYPE_BOOLEAN; };
template<> struct DBusController::Impl::DBusType< int16_t >       { constexpr static const int type = DBUS_TYPE_INT16; };
template<> struct DBusController::Impl::DBusType< uint16_t >      { constexpr static const int type = DBUS_TYPE_UINT16; };
template<> struct DBusController::Impl::DBusType< int32_t >       { constexpr static const int type = DBUS_TYPE_INT32; };
template<> struct DBusController::Impl::DBusType< uint32_t >      { constexpr static const int type = DBUS_TYPE_UINT32; };
template<> struct DBusController::Impl::DBusType< int64_t >       { constexpr static const int type = DBUS_TYPE_INT64; };
template<> struct DBusController::Impl::DBusType< uint64_t >      { constexpr static const int type = DBUS_TYPE_UINT64; };
template<> struct DBusController::Impl::DBusType< double >        { constexpr static const int type = DBUS_TYPE_DOUBLE; };


DBusController::DBusController( Processor & processor )
	: pImpl( new Impl(processor) )
{
	dbus_error_init( &(this->pImpl->error) );

	try
	{
		this->pImpl->connection = dbus_bus_get( DBUS_BUS_SYSTEM, &(this->pImpl->error) );
		if( dbus_error_is_set( &(this->pImpl->error) ) )
			throw RUNTIME_ERROR( "Connection Error: " + std::string(this->pImpl->error.message) );
		if( !this->pImpl->connection )
			throw RUNTIME_ERROR( "Could not connect to system DBus" );

		int ret = dbus_bus_request_name(
			this->pImpl->connection,
			dBusName,
			0,
			&(this->pImpl->error)
		);
		if( dbus_error_is_set( &(this->pImpl->error) ) )
			throw RUNTIME_ERROR( "Name Error: " + std::string(this->pImpl->error.message) );
		if( DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret )
			throw RUNTIME_ERROR( "Could not request name on system DBus" );
	} catch( ... )
	{
		// uninitialise dbus and rethrow exception
		if( this->pImpl->connection )
			dbus_connection_unref( this->pImpl->connection );
		dbus_error_free( &(this->pImpl->error) );
		throw;
	}
}


DBusController::~DBusController()
{
	if( this->pImpl->connection )
		dbus_connection_unref( this->pImpl->connection );
	dbus_error_free( &(this->pImpl->error) );
}


void DBusController::dispatch()
{
	while( true )
	{
		dbus_connection_read_write( this->pImpl->connection, 0 );

		DBusMessage * message = dbus_connection_pop_message( this->pImpl->connection );
		if( !message )
			return;

		if( dbus_message_get_type( message ) != DBUS_MESSAGE_TYPE_METHOD_CALL )
		{
			dbus_message_unref( message );
			continue;
		}

		std::string iface( dbus_message_get_interface( message ) );
		std::string member( dbus_message_get_member( message ) );

		std::cout << "DBusController: Called \"" << member << "\" on \"" << iface << "\"\n";

		// lookup called interface
		Impl::InterfaceMap::const_iterator ifaceit = this->pImpl->interfaceMap.find( iface );
		if( ifaceit == this->pImpl->interfaceMap.end() )
		{
			DBUS_ERROR( this->pImpl->connection, message, DBUS_ERROR_UNKNOWN_INTERFACE, "Interface \"" + iface + "\" is unknown" );
			dbus_message_unref( message );
			continue;
		}

		// lookup called method
		Impl::MethodMap::const_iterator methodit = ifaceit->second.find( member );
		if( methodit == ifaceit->second.end() )
		{
			DBUS_ERROR( this->pImpl->connection, message, DBUS_ERROR_UNKNOWN_METHOD, "Method \"" + member + "\" on interface \"" + iface + "\" is unknown" );
			dbus_message_unref( message );
			continue;
		}

		// call method
		methodit->second( this->pImpl->connection, message );

		dbus_message_unref( message );
	}
}
