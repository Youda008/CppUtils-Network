//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes for network addresses
//======================================================================================================================

#ifndef CPPUTILS_NETADDRESS_INCLUDED
#define CPPUTILS_NETADDRESS_INCLUDED


#include "Span.hpp"

#include <iosfwd>
#include <initializer_list>

// forward declaration of OS-dependent types
struct in_addr;
struct in6_addr;
struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct sockaddr_storage;


namespace own {


//======================================================================================================================
//  private implementation details

namespace impl {

	int genericCompare( const uint8_t * a1, const uint8_t * a2, size_t size );

	void genericCopy( const uint8_t * src, uint8_t * dst, size_t size );
	inline void fastCopy4(  const uint8_t * src, uint8_t * dst )
	{
		*reinterpret_cast< uint32_t * >( dst ) = *reinterpret_cast< const uint32_t * >( src );
	}
	inline void fastCopy6(  const uint8_t * src, uint8_t * dst )
	{
		reinterpret_cast< uint64_t * >( dst )[0] = reinterpret_cast< const uint64_t * >( src )[0];
		reinterpret_cast< uint64_t * >( dst )[1] = reinterpret_cast< const uint64_t * >( src )[1];
	}

	// system addresses are in the same byte order as ours
	inline void ownAddrToSysAddrV4( const uint8_t * ownAddr, struct in_addr * sysAddr )
	{
		fastCopy4( ownAddr, reinterpret_cast< uint8_t * >( sysAddr ));
	}
	inline void ownAddrToSysAddrV6( const uint8_t * ownAddr, struct in6_addr * sysAddr )
	{
		fastCopy6( ownAddr, reinterpret_cast< uint8_t * >( sysAddr ));
	}
	inline void sysAddrToOwnAddrV4( const struct in_addr * sysAddr, uint8_t * ownAddr )
	{
		fastCopy4( reinterpret_cast< const uint8_t * >( sysAddr ), ownAddr );
	}
	inline void sysAddrToOwnAddrV6( const struct in6_addr * sysAddr, uint8_t * ownAddr )
	{
		fastCopy6( reinterpret_cast< const uint8_t * >( sysAddr ), ownAddr );
	}

}


//======================================================================================================================

/** TODO */
template< size_t Size >
class GenericAddress
{

 protected:

	uint8_t _data [Size];

 public:

	GenericAddress() {}

	GenericAddress( fixed_const_byte_span< Size > data )
	{
		impl::genericCopy( data.data(), _data, Size );
	}
	GenericAddress( const GenericAddress< Size > & other )
	{
		impl::genericCopy( other._data, _data, Size );
	}
	GenericAddress< Size > & operator=( const GenericAddress< Size > & other )
	{
		impl::genericCopy( other._data, _data, Size );
		return *this;
	}

	fixed_byte_span< Size > data()  { return fixed_byte_span< Size >( _data ); }
	fixed_const_byte_span< Size > data() const  { return fixed_const_byte_span< Size >( _data ); }

	uint8_t operator[]( size_t idx )        { return _data[ idx ]; }
	uint8_t operator[]( size_t idx ) const  { return _data[ idx ]; }

	bool operator==( const GenericAddress< Size > & other ) const { return impl::genericCompare( _data, other._data ) == 0; }
	bool operator< ( const GenericAddress< Size > & other ) const { return impl::genericCompare( _data, other._data ) < 0; }
	bool operator> ( const GenericAddress< Size > & other ) const { return impl::genericCompare( _data, other._data ) > 0; }

};


class IPAddr;

enum class IPVer
{
	_4 = 4,
	_6 = 6
};


/** TODO */
class IPv4Addr : public GenericAddress<4>
{
	friend class IPAddr;

 public:

	using GenericAddress<4>::GenericAddress;

	// single instruction construction and assignment
	IPv4Addr( fixed_const_byte_span<4> data )
	{
		impl::fastCopy4( data.data(), _data );
	}
	IPv4Addr( const IPv4Addr & other ) : GenericAddress<4>()
	{
		impl::fastCopy4( other.data().data(), _data );
	}
	IPv4Addr & operator=( const IPv4Addr & other )
	{
		impl::fastCopy4( other.data().data(), _data );
		return *this;
	}

	friend std::ostream & operator<<( std::ostream & os, IPv4Addr addr );
	friend std::istream & operator>>( std::istream & is, IPv4Addr & addr );
};


/** TODO */
class IPv6Addr : public GenericAddress<16>
{
	friend class IPAddr;

 public:

	using GenericAddress<16>::GenericAddress;

	IPv6Addr( fixed_const_byte_span<4> data )
	{
		impl::fastCopy6( data.data(), _data );
	}
	IPv6Addr( const IPv6Addr & other ) : GenericAddress<16>()
	{
		impl::fastCopy6( other.data().data(), _data );
	}
	IPv6Addr & operator=( const IPv6Addr & other )
	{
		impl::fastCopy6( other.data().data(), _data );
		return *this;
	}

	friend std::ostream & operator<<( std::ostream & os, const IPv6Addr & addr );
	friend std::istream & operator>>( std::istream & is, IPv6Addr & addr );
};


/** TODO */
class IPAddr : public GenericAddress<16>
{
	IPVer _version;

 public:

	using GenericAddress<16>::GenericAddress;

	IPAddr() : _version( static_cast<IPVer>(0) ) {}

	IPAddr( const_byte_span data );
	IPAddr( std::initializer_list< uint8_t > initList );

	IPAddr( fixed_const_byte_span<4> data )
	{
		impl::fastCopy4( data.data(), _data );
		_version = IPVer::_4;
	}
	IPAddr( fixed_const_byte_span<16> data )
	{
		impl::fastCopy6( data.data(), _data );
		_version = IPVer::_6;
	}
	IPAddr( const IPv4Addr & addr )
	{
		impl::fastCopy4( addr._data, _data );
		_version = IPVer::_4;
	}
	IPAddr( const IPv6Addr & addr )
	{
		impl::fastCopy6( addr._data, _data );
		_version = IPVer::_6;
	}

	IPVer version() const  { return _version; }

	IPv4Addr v4() const;
	IPv6Addr v6() const;

	friend std::ostream & operator<<( std::ostream & os, const IPAddr & addr );
	friend std::istream & operator>>( std::istream & is, IPAddr & addr );
};


//======================================================================================================================

class MACAddr : public GenericAddress<6>
{
 public:

	using GenericAddress<6>::GenericAddress;

	friend std::ostream & operator<<( std::ostream & os, const MACAddr & addr );
	friend std::istream & operator>>( std::istream & is, MACAddr & addr );
};


//======================================================================================================================

struct Endpoint
{
	IPAddr addr;
	uint16_t port;
};

void endpointToSockaddr( const Endpoint & ep, struct sockaddr * saddr, int & addrlen );

void sockaddrToEndpoint( const struct sockaddr * saddr, Endpoint & ep );


//======================================================================================================================


} // namespace own


#endif // CPPUTILS_NETADDRESS_INCLUDED
