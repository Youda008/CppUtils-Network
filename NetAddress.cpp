//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes for network addresses
//======================================================================================================================

#include "NetAddress.hpp"

#include "LangUtils.hpp"  // make_reverse_iterator
using own::span;

#include <cstring>  // memset
#include <string>
#include <ostream>
#include <istream>
#include <stdexcept>


namespace own {


using namespace impl;


//======================================================================================================================
//  utils impl

namespace impl {

void copyAddr( const uint8_t * src, uint8_t * dst, size_t size )
{
	memcpy( dst, src, size );
}

int compareAddr( const uint8_t * a1, const uint8_t * a2, size_t size )
{
	return std::lexicographical_compare(
		own::make_reverse_iterator( a1 ),
		own::make_reverse_iterator( a1 + size ),
		own::make_reverse_iterator( a2 ),
		own::make_reverse_iterator( a2 + size )
	);
}

} // namespace impl


//======================================================================================================================
//  IPv4Addr

IPv4Addr::IPv4Addr( span< const uint8_t > data )
{
	if (data.size() != sizeof(data))
	{
		throw std::invalid_argument(
			"IPv4 address can only be constructed from a buffer of size 4, current size: "+std::to_string( data.size() )
		);
	}
	copyAddr( data.data(), _data, sizeof(data) );
}

std::ostream & operator<<( std::ostream & os, IPv4Addr addr )
{
	os << uint(addr[0]) << '.' << uint( addr[1] ) << '.' << uint( addr[2] ) << '.' << uint( addr[3] );
	return os;
}

std::ostream & operator>>( std::istream & /*os*/, IPv4Addr & /*addr*/ )
{
	TODO
}


//======================================================================================================================
//  IPv6Addr

IPv6Addr::IPv6Addr( span< const uint8_t > data )
{
	if (data.size() != sizeof(data))
	{
		throw std::invalid_argument(
			"IPv6 address can only be constructed from a buffer of size 16, current size: "+std::to_string( data.size() )
		);
	}
	copyAddr( data.data(), _data, sizeof(data) );
}

std::ostream & operator<<( std::ostream & /*os*/, const IPv6Addr & /*addr*/ )
{
	TODO
}

std::ostream & operator>>( std::istream & /*os*/, IPv6Addr & /*addr*/ )
{
	TODO
}


//======================================================================================================================
//  IPAddr

// TODO: construct from fixed span with compile-time length
IPAddr::IPAddr( span< const uint8_t > data )
{
	if (data.size() == 4)
	{
		_version = Ver::_4;
		copyAddr( data.data(), _data, 4 );
	}
	else if (data.size() == 16)
	{
		_version = Ver::_6;
		copyAddr( data.data(), _data, 16 );
	}
	else
	{
		throw std::invalid_argument(
			"IP address can only be constructed from a buffer of size 4 or 16, current size: "+std::to_string( data.size() )
		);
	}
}

IPAddr::IPAddr( std::initializer_list< uint8_t > initList )
{
	if (initList.size() == 4)
	{
		_version = Ver::_4;
		copyAddr( &*initList.begin(), _data, 4 );
	}
	else if (initList.size() == 16)
	{
		_version = Ver::_6;
		copyAddr( &*initList.begin(), _data, 16 );
	}
	else
	{
		throw std::invalid_argument(
			"IP address can only be constructed from a buffer of size 4 or 16, current size: "+std::to_string( initList.size() )
		);
	}
}

std::ostream & operator<<( std::ostream & os, const IPAddr & addr )
{
	if (addr.version() == IPAddr::Ver::_4)
	{
		os << uint(addr[0]) << '.' << uint( addr[1] ) << '.' << uint( addr[2] ) << '.' << uint( addr[3] );
	}
	else
	{
		TODO
	}
	return os;
}

std::ostream & operator>>( std::istream & /*os*/, IPAddr & /*addr*/ )
{
	TODO
}


//======================================================================================================================


} // namespace own
