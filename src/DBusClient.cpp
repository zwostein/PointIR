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

#include "DBusClient.hpp"

#include <iostream>
#include <stdexcept>
#include <system_error>


#define SYSTEM_ERROR( errornumber, whattext ) \
std::system_error( (errornumber), std::system_category(), std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )

#define RUNTIME_ERROR( whattext ) \
std::runtime_error( std::string(__PRETTY_FUNCTION__) + std::string(": ") + (whattext) )


using namespace PointIR;


constexpr static const char * dBusName = "PointIR.Calibrator";
constexpr static const char * dBusControllerName = "PointIR.Controller";
constexpr static const char * dBusControllerObject = "/PointIR/Controller";


DBusClient::DBusClient()
{
	dbus_error_init( &this->dBusError );
	try
	{
		this->dBusConnection = dbus_bus_get( DBUS_BUS_SYSTEM, &(this->dBusError) );
		if( dbus_error_is_set( &this->dBusError ) )
			throw RUNTIME_ERROR( "Connection Error: " + std::string(this->dBusError.message) );
		if( !this->dBusConnection )
			throw RUNTIME_ERROR( "Could not connect to system DBus" );

		int ret = dbus_bus_request_name(
			this->dBusConnection,
			dBusName,
			0,
			&this->dBusError
		);
		if( dbus_error_is_set( &this->dBusError ) )
			throw RUNTIME_ERROR( "Name Error: " + std::string(this->dBusError.message) );
		if( DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret )
			throw RUNTIME_ERROR( "Could not request name on system DBus" );
	} catch( ... )
	{
		// uninitialise dbus and rethrow exception
		if( this->dBusConnection )
			dbus_connection_unref( this->dBusConnection );
		dbus_error_free( &this->dBusError );
		throw;
	}
}


DBusClient::~DBusClient()
{
	if( this->dBusConnection )
		dbus_connection_unref( this->dBusConnection );
	dbus_error_free( &this->dBusError );
}


bool DBusClient::getBool( const std::string & interface, const std::string & method ) const
{
	DBusMessage * msg = dbus_message_new_method_call(
		dBusControllerName,    // target for the method call
		dBusControllerObject, // object to call on
		interface.c_str(),   // interface to call on
		method.c_str()      // method name
	);
	if( !msg )
		throw RUNTIME_ERROR( "dbus_message_new_method_call failed" );

	try
	{
		// append arguments
		DBusMessageIter args;
		dbus_message_iter_init_append( msg, &args );

		// send message and get a handle for a reply
		DBusMessage * reply = dbus_connection_send_with_reply_and_block( this->dBusConnection, msg, -1, &this->dBusError ); // -1 is default timeout
		if( !reply )
			throw RUNTIME_ERROR( "dbus_connection_send_with_reply_and_block failed: " + std::string(this->dBusError.message) );

		try
		{
			// handle arguments in reply
			DBusBasicValue value;
			if( !dbus_message_iter_init( reply, &args ) )
				throw RUNTIME_ERROR( "Expected one argument in reply" );
			if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_BOOLEAN )
				throw RUNTIME_ERROR( "Expected argument of type bool" );
			dbus_message_iter_get_basic( &args, &value );
			bool result = value.bool_val;

			// free messages
			dbus_message_unref( reply );
			dbus_message_unref( msg );
			return result;
		}
		catch( ... )
		{
			dbus_message_unref( reply );
			throw;
		}
	}
	catch( ... )
	{
		dbus_message_unref( msg );
		throw;
	}
}


std::string DBusClient::getCalibrationImageFile( unsigned int width, unsigned int height ) const
{
	DBusMessage * msg = dbus_message_new_method_call(
		dBusControllerName,               // target for the method call
		dBusControllerObject,             // object to call on
		"PointIR.Controller.Unprojector", // interface to call on
		"generateCalibrationImageFile"    // method name
	);
	if( !msg )
		throw RUNTIME_ERROR( "dbus_message_new_method_call failed" );

	try
	{
		// append arguments
		DBusMessageIter args;
		dbus_message_iter_init_append( msg, &args );

		DBusBasicValue value;
		value.u32 = width;
		if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, &value ) )
			throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );
		value.u32 = height;
		if( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, &value ) )
			throw RUNTIME_ERROR( "dbus_message_iter_append_basic failed" );

		// send message and get a handle for a reply
		DBusMessage * reply = dbus_connection_send_with_reply_and_block( this->dBusConnection, msg, -1, &this->dBusError ); // -1 is default timeout
		if( !reply )
			throw RUNTIME_ERROR( "dbus_connection_send_with_reply_and_block failed: " + std::string(this->dBusError.message) );

		try
		{
			// handle arguments in reply
			if( !dbus_message_iter_init( reply, &args ) )
				throw RUNTIME_ERROR( "Expected one argument in reply" );
			if( dbus_message_iter_get_arg_type( &args ) != DBUS_TYPE_STRING )
				throw RUNTIME_ERROR( "Expected argument of type string" );
			dbus_message_iter_get_basic( &args, &value );
			std::string filename( value.str );

			// free messages
			dbus_message_unref( msg );
			dbus_message_unref( reply );

			return filename;
		}
		catch( ... )
		{
			dbus_message_unref( reply );
			throw;
		}
	}
	catch( ... )
	{
		dbus_message_unref( msg );
		throw;
	}
}


bool DBusClient::calibrate() const
{
	return this->getBool( "PointIR.Controller.Processor", "calibrate" );
}


bool DBusClient::saveCalibrationData() const
{
	return this->getBool( "PointIR.Controller.Unprojector", "saveCalibrationData" );
}
