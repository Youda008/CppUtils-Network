//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes for network addresses
//======================================================================================================================

#include "NetAddress.hpp"

#include "Essential.hpp"

#include "CriticalError.hpp"
using own::span;

#include <cstring>  // memset
#include <string>
#include <ostream>
#include <istream>

// string <-> binary conversions
#ifdef _WIN32
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>
#endif


namespace own {


using namespace impl;


//======================================================================================================================
//  utils impl

namespace impl {

void genericCopy( const uint8_t * src, uint8_t * dst, size_t size ) noexcept
{
	memcpy( dst, src, size );
}

int genericCompare( const uint8_t * a1, const uint8_t * a2, size_t size ) noexcept
{
	return std::lexicographical_compare( a1, a1 + size, a2, a2 + size );
}

static std::ostream & ipv4ToStream( std::ostream & os, const uint8_t * bytes )
{
	char str [16];
	struct in_addr addr;
	impl::ownAddrToSysAddrV4( bytes, &addr );
	if (!inet_ntop( AF_INET, &addr, str, sizeof(str) ))
		critical_error( "Failed to convert IPv4 address to string." );
	os << str;
	return os;
}

static std::ostream & ipv6ToStream( std::ostream & os, const uint8_t * bytes )
{
	char str [40];
	struct in6_addr addr;
	impl::ownAddrToSysAddrV6( bytes, &addr );
	if (!inet_ntop( AF_INET6, &addr, str, sizeof(str) ))
		critical_error( "Failed to convert IPv6 address to string." );
	os << str;
	return os;
}

static std::istream & ipv4FromStream( std::istream & is, uint8_t * bytes )
{
	std::string ipStr;
	if (!(is >> ipStr))
		return is;
	struct in_addr addr;
	if (inet_pton( AF_INET, ipStr.c_str(), &addr ) != 1)
		is.setstate( std::ios::failbit );
	impl::sysAddrToOwnAddrV4( &addr, bytes );
	return is;
}

static std::istream & ipv6FromStream( std::istream & is, uint8_t * bytes )
{
	std::string ipStr;
	if (!(is >> ipStr))
		return is;
	struct in6_addr addr;
	if (inet_pton( AF_INET, ipStr.c_str(), &bytes ) != 1)
		is.setstate( std::ios::failbit );
	impl::sysAddrToOwnAddrV6( &addr, bytes );
	return is;
}

static IPVer ipAnyFromStream( std::istream & is, uint8_t * bytes )
{
	std::string ipStr;
	if (!(is >> ipStr))
		return static_cast<IPVer>(0);
	struct in_addr addr4;
	struct in6_addr addr6;
	if (inet_pton( AF_INET, ipStr.c_str(), &addr4 ) == 1)
	{
		impl::sysAddrToOwnAddrV4( &addr4, bytes );
		return IPVer::_4;
	}
	else if (inet_pton( AF_INET6, ipStr.c_str(), &addr6 ) == 1)
	{
		impl::sysAddrToOwnAddrV6( &addr6, bytes );
		return IPVer::_6;
	}
	else
	{
		is.setstate( std::ios::failbit );
		return static_cast<IPVer>(0);
	}
}

} // namespace impl


//======================================================================================================================
//  IPv4Addr

std::ostream & operator<<( std::ostream & os, IPv4Addr addr )
{
	return impl::ipv4ToStream( os, addr.data().data() );
}

std::istream & operator>>( std::istream & is, IPv4Addr & addr )
{
	return impl::ipv4FromStream( is, addr.data().data() );
}


//======================================================================================================================
//  IPv6Addr

std::ostream & operator<<( std::ostream & os, const IPv6Addr & addr )
{
	return impl::ipv6ToStream( os, addr.data().data() );
}

std::istream & operator>>( std::istream & is, IPv6Addr & addr )
{
	return impl::ipv6FromStream( is, addr.data().data() );
}


//======================================================================================================================
//  IPAddr

IPAddr::IPAddr( std::initializer_list< uint8_t > initList )
{
	if (initList.size() == 4)
	{
		impl::fastCopy4( initList.begin(), _data );
		_version = IPVer::_4;
	}
	else if (initList.size() == 16)
	{
		impl::fastCopy16( initList.begin(), _data );
		_version = IPVer::_6;
	}
	else
	{
		critical_error(
			"IP address can only be constructed from a buffer of size 4 or 16, current size: %zu", initList.size()
		);
	}
}

IPAddr::IPAddr( const_byte_span data )
{
	if (data.size() == 4)
	{
		impl::fastCopy4( data.data(), _data );
		_version = IPVer::_4;
	}
	else if (data.size() == 16)
	{
		impl::fastCopy16( data.data(), _data );
		_version = IPVer::_6;
	}
	else
	{
		critical_error(
			"IP address can only be constructed from a buffer of size 4 or 16, current size: %zu", data.size()
		);
	}
}

std::ostream & operator<<( std::ostream & os, const IPAddr & addr )
{
	if (addr.version() == IPVer::_4)
		return impl::ipv4ToStream( os, addr.data().data() );
	else if (addr.version() == IPVer::_6)
		return impl::ipv6ToStream( os, addr.data().data() );
	else
		critical_error( "Attempted to print uninitialized IPAddr." );
}

std::istream & operator>>( std::istream & is, IPAddr & addr )
{
	addr._version = impl::ipAnyFromStream( is, addr.data().data() );
	return is;
}


//======================================================================================================================

std::ostream & operator<<( std::ostream & /*os*/, const MACAddr & /*addr*/ )
{
	TODO
}

std::istream & operator>>( std::istream & /*os*/, MACAddr & /*addr*/ )
{
	TODO
}


//======================================================================================================================
//  Endpoint utils

void endpointToSockaddr( const Endpoint & ep, struct sockaddr * saddr, int & addrlen )
{
	memset( saddr, 0, sizeof(*saddr) );
	if (ep.addr.version() == IPVer::_4)
	{
		auto saddr4 = reinterpret_cast< struct sockaddr_in * >( saddr );
		saddr4->sin_family = AF_INET;
		impl::ownAddrToSysAddrV4( ep.addr.data().data(), &saddr4->sin_addr );
		saddr4->sin_port = htons( ep.port );
		addrlen = sizeof(sockaddr_in);
	}
	else if (ep.addr.version() == IPVer::_6)
	{
		auto saddr6 = reinterpret_cast< struct sockaddr_in6 * >( saddr );
		saddr6->sin6_family = AF_INET6;
		impl::ownAddrToSysAddrV6( ep.addr.data().data(), &saddr6->sin6_addr );
		saddr6->sin6_port = htons( ep.port );
		addrlen = sizeof(sockaddr_in6);
	}
	else
	{
		critical_error( "Attempted socket operation with uninitialized IPAddr." );
	}
}

void sockaddrToEndpoint( const struct sockaddr * saddr, Endpoint & ep )
{
	if (saddr->sa_family == AF_INET)
	{
		auto saddr4 = reinterpret_cast< const struct sockaddr_in * >( saddr );
		impl::sysAddrToOwnAddrV4( &saddr4->sin_addr, ep.addr.data().data() );
		ep.port = ntohs( saddr4->sin_port );
	}
	else if (saddr->sa_family == AF_INET6)
	{
		auto saddr6 = reinterpret_cast< const struct sockaddr_in6 * >( saddr );
		impl::sysAddrToOwnAddrV6( &saddr6->sin6_addr, ep.addr.data().data() );
		ep.port = ntohs( saddr6->sin6_port );
	}
	else
	{
		critical_error( "Socket operation returned unexpected address family." );
	}
}


//======================================================================================================================


} // namespace own
